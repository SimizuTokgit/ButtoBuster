#include "EnemyIdleState.h"
#include "Enemy.h"
#include "EnemyActions.h"
#include "EnemyMoveState.h"
#include <memory>

using std::make_unique;

void EnemyIdleState::Enter(Enemy& enemy) {
    enemy.PlayAnimation("Idle");
    enemy.StopHorizontal();
}

void EnemyIdleState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    enemy.FaceTowards(input.look, enemy.GetData().turnSpeed, deltaTime);

    if (EnemyActions::TryStartAttack(enemy, this, input.technique)) return;

    if (VSize(input.move) > 0.05f) {
        enemy.GetStates().Transition(this, make_unique<EnemyMoveState>());
        return;
    }

    enemy.StopHorizontal();
}
