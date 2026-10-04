#include "EnemyBlowState.h"
#include "Enemy.h"
#include "EnemyIdleState.h"
#include "BlowChain.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void EnemyBlowState::Enter(Enemy& enemy) {
    const EnemyData& data = enemy.GetData();
    enemy.SetKnockback(_knockback);

    if (data.isFlying) {
        // 羽ばたきを止めて落とす
        enemy.SetHovering(false);
        enemy.PlayAnimation("Damage", 1.0f, true);
        enemy.SetVerticalVelocity(250.0f);
    }
    else if (data.canBlow) {
        enemy.PlayAnimation("BlowIn", 1.0f, true);
        enemy.SetVerticalVelocity(380.0f);
        enemy.FaceImmediately(VScale(_knockback, -1.0f));
    }
    else {
        // 吹き飛ぶアニメを持たない重い敵は、構えたまま押し飛ばされる
        enemy.PlayAnimation("Idle", 1.0f, true);
        enemy.SetVerticalVelocity(HEAVY_JUMP_SPEED);
    }

    if (_chain) _chain->BeginFlight(enemy);
    _phase = Phase::Fly;
}

void EnemyBlowState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    const EnemyData& data = enemy.GetData();
    bool isFlying = data.isFlying;

    switch (_phase) {
    case Phase::Fly: {
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

    case Phase::Down: {
        _timer += deltaTime;
        float downTime = isFlying ? GROUNDED_TIME : (data.canBlow ? DOWN_TIME : HEAVY_RECOVER_TIME);
        if (_timer < downTime) break;

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
