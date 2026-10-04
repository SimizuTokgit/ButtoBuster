#include "EnemyMoveState.h"
#include "Enemy.h"
#include "EnemyActions.h"
#include "EnemyIdleState.h"
#include <memory>

using std::make_unique;

void EnemyMoveState::Enter(Enemy& enemy) {
    _isRunning = false;
    enemy.PlayAnimation("Walk");
}

void EnemyMoveState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    if (EnemyActions::TryStartAttack(enemy, this, input.technique)) return;

    float amount = VSize(input.move);
    if (amount <= 0.05f) {
        enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
        return;
    }

    bool isRunning = amount > RUN_THRESHOLD;
    if (isRunning != _isRunning) {
        _isRunning = isRunning;
        enemy.PlayAnimation(isRunning ? "Run" : "Walk");
    }

    const EnemyData& data = enemy.GetData();

    // 歩きは倒し具合 RUN_THRESHOLD でちょうど歩きの速さになるようにする
    float speed = isRunning ? data.runSpeed : data.walkSpeed / RUN_THRESHOLD;
    enemy.SetHorizontalVelocity(input.move, speed);

    // 回り込むときは横に歩きながら相手を見続ける
    VECTOR facing = (VSize(input.look) > 0.001f) ? input.look : input.move;
    enemy.FaceTowards(facing, data.turnSpeed, deltaTime);
}
