#include "EnemyBlowState.h"
#include "Enemy.h"
#include "EnemyIdleState.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void EnemyBlowState::Enter(Enemy& enemy) {
    enemy.SetKnockback(_knockback);

    if (enemy.GetData().isFlying) {
        // 羽ばたきを止めて落とす
        enemy.SetHovering(false);
        enemy.PlayAnimation("Damage", 1.0f, true);
        enemy.SetVerticalVelocity(250.0f);
    }
    else {
        enemy.PlayAnimation("BlowIn", 1.0f, true);
        enemy.SetVerticalVelocity(380.0f);
        enemy.FaceImmediately(VScale(_knockback, -1.0f));
    }
    _phase = Phase::Fly;
}

void EnemyBlowState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    bool isFlying = enemy.GetData().isFlying;

    switch (_phase) {
    case Phase::Fly: {
        enemy.DampHorizontal(1.5f, deltaTime);

        bool hasLanded = enemy.IsGrounded() && enemy.GetVelocity().y <= 0.0f;
        bool isAnimationDone = isFlying || enemy.IsAnimationFinished();
        if (hasLanded && isAnimationDone) {
            enemy.StopHorizontal();
            if (!isFlying) enemy.PlayAnimation("DownLoop");
            if (auto* effects = EffectManager::Get()) effects->PlayDust(enemy.GetPosition(), 8);
            _phase = Phase::Down;
            _timer = 0.0f;
        }
        break;
    }

    case Phase::Down:
        _timer += deltaTime;
        if (_timer < (isFlying ? GROUNDED_TIME : DOWN_TIME)) break;

        if (isFlying) {
            // 飛び直す 浮く高さへはホバリングの力で戻っていく
            enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
        }
        else {
            enemy.PlayAnimation("BlowOut", 1.2f, true);
            _phase = Phase::GetUp;
        }
        break;

    case Phase::GetUp:
        if (enemy.IsAnimationFinished()) {
            enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
        }
        break;
    }
}

void EnemyBlowState::Exit(Enemy& enemy) {
    if (enemy.GetData().isFlying && !enemy.IsDead()) {
        enemy.SetHovering(true);
    }
}
