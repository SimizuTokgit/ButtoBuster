#include "EnemyDamageState.h"
#include "Enemy.h"
#include "EnemyIdleState.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void EnemyDamageState::Enter(Enemy& enemy) {
    enemy.PlayAnimation("Damage", 1.2f, true);
    enemy.SetKnockback(VScale(_knockback, 0.6f));
}

void EnemyDamageState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    enemy.DampHorizontal(8.0f, deltaTime);

    // 叩きつけで落とされたら、地面に着いた所で跳ね上がる 浮いたところへ続けて斬れるように
    if (_isSlammed && enemy.IsGrounded()) {
        _isSlammed = false;
        enemy.SetVerticalVelocity(SLAM_BOUNCE_SPEED);
        if (auto* effects = EffectManager::Get()) effects->PlayDust(enemy.GetPosition(), 8);
        return;
    }

    // 空中の斬りで浮かされている間は、のけぞったまま落ちる 宙で歩き出したり攻撃したりしないように
    // 浮いている敵は地面に着かないので待たない
    bool canRecover = enemy.GetData().isFlying || enemy.IsGrounded();
    if (!canRecover) return;

    if (enemy.GetAnimationTime() > 12.0f || enemy.IsAnimationFinished()) {
        enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
    }
}
