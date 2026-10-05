#include "EnemyBlowState.h"
#include "Enemy.h"
#include "EnemyIdleState.h"
#include "BlowChain.h"
#include "ArenaWall.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void EnemyBlowState::Enter(Enemy& enemy) {
    const EnemyData& data = enemy.GetData();
    enemy.SetKnockback(_knockback);
    enemy.ForgetWallHit();

    // 吹っ飛ぶ角度 (Character::LAUNCH_ANGLE) で斜め上へ飛ばす 弱く飛ばされても、下の速さだけは跳ねる
    if (data.isFlying) {
        // 羽ばたきを止めて落とす
        enemy.SetHovering(false);
        enemy.PlayAnimation("Damage", 1.0f, true);
        enemy.SetVerticalVelocity(Character::GetLaunchUpSpeed(_knockback, 250.0f));
    }
    else if (data.canBlow) {
        enemy.PlayAnimation("BlowIn", 1.0f, true);
        enemy.SetVerticalVelocity(Character::GetLaunchUpSpeed(_knockback, 380.0f));
        enemy.FaceImmediately(VScale(_knockback, -1.0f));
    }
    else {
        // 吹き飛ぶアニメを持たない重い敵は、構えたまま押し飛ばされる
        enemy.PlayAnimation("Idle", 1.0f, true);
        enemy.SetVerticalVelocity(Character::GetLaunchUpSpeed(_knockback, HEAVY_JUMP_SPEED));
    }

    if (_chain) _chain->BeginFlight(enemy);
    _phase = Phase::Fly;
}

void EnemyBlowState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    const EnemyData& data = enemy.GetData();
    bool isFlying = data.isFlying;

    switch (_phase) {
    case Phase::Fly: {
        // 飛んでいる間は吹っ飛ばされ値を減らさない
        enemy.HoldBlow();

        // 壁にぶつかったら、吹っ飛ばされ値が許容値に届いていれば壁を割り、届いていなければ張り付いてから跳ね返る
        ArenaWall::Hit wallHit;
        if (enemy.ConsumeWallHit(wallHit)) {
            ArenaWall::Reaction reaction = ArenaWall::React(enemy, wallHit);

            // 割ったときは場外へ飛んでいく状態に移るので、ここから先はいらない
            if (reaction == ArenaWall::Reaction::Break) break;

            if (reaction == ArenaWall::Reaction::Bounce) {
                // ぶつかった姿のまま壁に止める 砲弾はやめずにおき、跳ね返ったあとも連鎖を続ける
                _wallHit = wallHit;
                _stickTime = ArenaWall::GetStickTime(wallHit);
                _timer = 0.0f;
                enemy.StopHorizontal();
                enemy.SetVerticalVelocity(0.0f);
                enemy.SetAnimationSpeed(0.0f);
                _phase = Phase::Stick;
                break;
            }
        }

        enemy.DampHorizontal(1.5f, deltaTime);

        // 速いうちは砲弾として、触れた敵を巻き込む
        if (_chain && !_chain->Sweep(enemy, deltaTime)) LeaveChain(enemy);

        bool hasLanded = enemy.IsGrounded() && enemy.GetVelocity().y <= 0.0f;
        bool isAnimationDone = isFlying || !data.canBlow || enemy.IsAnimationFinished();
        if (hasLanded && isAnimationDone) {
            LeaveChain(enemy);
            enemy.StopHorizontal();
            if (!isFlying && data.canBlow) enemy.PlayAnimation("DownLoop");
            if (auto* effects = EffectManager::Get()) effects->PlayDust(enemy.GetPosition(), 8);
            _phase = Phase::Down;
            _timer = 0.0f;
        }
        break;
    }

    case Phase::Stick: {
        enemy.HoldBlow();

        // 張り付いている間は壁に止めておく ここで追い打ちを当てられる
        enemy.StopHorizontal();
        enemy.SetVerticalVelocity(0.0f);

        _timer += deltaTime;
        if (_timer < _stickTime) break;

        // 速く跳ね返れば、戻ってくる間も砲弾のままなので、群れの中へ連鎖が続く
        ArenaWall::Bounce(enemy, _wallHit);
        if (!isFlying && data.canBlow) {
            // 飛ばされた向きの逆を向いて飛ぶのは、跳ね返ったあとも同じ
            enemy.PlayAnimation("BlowIn", 1.0f, true);
            enemy.FaceImmediately(VScale(enemy.GetVelocity(), -1.0f));
        }
        else {
            enemy.SetAnimationSpeed(1.0f);
        }
        _phase = Phase::Fly;
        break;
    }

    case Phase::Down: {
        _timer += deltaTime;
        float downTime = isFlying ? GROUNDED_TIME : (data.canBlow ? DOWN_TIME : HEAVY_RECOVER_TIME);
        if (_timer < downTime) break;

        // 起き上がるときに吹っ飛ばされ値を少し戻す 起き上がりに当て続けるだけでは溜まりきらないように
        enemy.RecoverBlowOnGetUp();

        // 飛ぶ敵は飛び直す 浮く高さへはホバリングの力で戻っていく
        // 重い敵は起き上がるアニメが無いので、そのまま動き出す
        if (isFlying || !data.canBlow) {
            enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
        }
        else {
            enemy.PlayAnimation("BlowOut", 1.2f, true);
            _phase = Phase::GetUp;
        }
        break;
    }

    case Phase::GetUp:
        if (enemy.IsAnimationFinished()) {
            enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
        }
        break;
    }
}

void EnemyBlowState::Exit(Enemy& enemy) {
    LeaveChain(enemy);

    if (enemy.GetData().isFlying && !enemy.IsDead()) {
        enemy.SetHovering(true);
    }
}

void EnemyBlowState::LeaveChain(Enemy& enemy) {
    if (!_chain) return;

    _chain->EndFlight(enemy.GetPosition());
    _chain.reset();
}
