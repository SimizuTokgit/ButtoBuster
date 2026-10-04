#include "EnemyDeadState.h"
#include "Enemy.h"
#include "BlowChain.h"
#include "ArenaWall.h"
#include "EffectManager.h"
#include "Transform.h"

void EnemyDeadState::Enter(Enemy& enemy) {
    const EnemyData& data = enemy.GetData();

    // 倒れた体で生きている相手を押さない
    enemy.SetBodySolid(false);
    enemy.SetKnockback(_knockback);
    enemy.ForgetWallHit();

    if (data.isFlying) {
        enemy.SetHovering(false);
        enemy.PlayAnimation("Down", 1.0f, true);
        enemy.SetVerticalVelocity(150.0f);
    }
    else if (data.canBlow) {
        enemy.PlayAnimation("BlowIn", 1.0f, true);
        enemy.SetVerticalVelocity(380.0f);
        enemy.FaceImmediately(VScale(_knockback, -1.0f));
    }
    else {
        enemy.PlayAnimation("Down", 1.0f, true);
        if (_isBlown) {
            enemy.SetVerticalVelocity(HEAVY_JUMP_SPEED);
        }
        else {
            enemy.StopHorizontal();
        }
    }

    if (_chain && _isBlown) _chain->BeginFlight(enemy);
    _phase = Phase::Fall;
    _timer = 0.0f;
}

void EnemyDeadState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    // 地形に引っかかって着地できなくても、いつまでも残らないようにする
    constexpr float FALL_TIME_LIMIT = 3.0f;
    constexpr float SINK_SPEED = 120.0f;

    const EnemyData& data = enemy.GetData();
    _timer += deltaTime;

    switch (_phase) {
    case Phase::Fall: {
        // 倒れながら飛ばされても、壁は抜けずに跳ね返る
        ArenaWall::Hit wallHit;
        if (enemy.ConsumeWallHit(wallHit) && ArenaWall::TryBounce(enemy, wallHit)) {
            if (data.canBlow && !data.isFlying) {
                enemy.PlayAnimation("BlowIn", 1.0f, true);
                enemy.FaceImmediately(VScale(enemy.GetVelocity(), -1.0f));
            }
        }

        enemy.DampHorizontal(2.0f, deltaTime);

        // 倒れた敵も、速いうちは砲弾として触れた敵を巻き込む
        if (_chain && !_chain->Sweep(enemy, deltaTime)) LeaveChain(enemy);

        bool hasLanded = enemy.IsGrounded() && enemy.GetVelocity().y <= 0.0f;
        bool isAnimationDone = data.isFlying || enemy.IsAnimationFinished();
        if ((hasLanded && isAnimationDone) || _timer > FALL_TIME_LIMIT) {
            LeaveChain(enemy);
            enemy.StopHorizontal();
            if (data.canBlow && !data.isFlying) enemy.PlayAnimation("DownLoop");
            if (auto* effects = EffectManager::Get()) {
                effects->PlayDeath(enemy.GetCenter());
                effects->PlayDust(enemy.GetPosition(), 10);
            }
            _phase = Phase::Lie;
            _timer = 0.0f;
        }
        break;
    }

    case Phase::Lie:
        if (_timer > Enemy::CORPSE_TIME) {
            // 物理を止めて自分で沈める 重力と地形の押し戻しが効いていると沈まない
            enemy.SetKinematic(true);
            _phase = Phase::Sink;
            _timer = 0.0f;
        }
        break;

    case Phase::Sink: {
        float rate = _timer / Enemy::SINK_TIME;
        enemy.transform->localPosition = VAdd(enemy.transform->localPosition, VGet(0.0f, -SINK_SPEED * deltaTime, 0.0f));
        enemy.SetOpacity(1.0f - rate);
        if (rate >= 1.0f) enemy.MarkReadyToRemove();
        break;
    }
    }
}

void EnemyDeadState::LeaveChain(Enemy& enemy) {
    if (!_chain) return;

    _chain->EndFlight(enemy.GetPosition());
    _chain.reset();
}
