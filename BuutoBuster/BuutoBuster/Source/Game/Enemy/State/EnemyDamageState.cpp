#include "EnemyDamageState.h"
#include "Enemy.h"
#include "EnemyIdleState.h"
#include <memory>

using std::make_unique;

void EnemyDamageState::Enter(Enemy& enemy) {
    enemy.PlayAnimation("Damage", 1.2f, true);
    enemy.SetKnockback(VScale(_knockback, 0.6f));
}

void EnemyDamageState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    enemy.DampHorizontal(8.0f, deltaTime);

    if (enemy.GetAnimationTime() > 12.0f || enemy.IsAnimationFinished()) {
        enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
    }
}
