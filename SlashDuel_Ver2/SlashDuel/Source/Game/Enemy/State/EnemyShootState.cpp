#include "EnemyShootState.h"
#include "Enemy.h"
#include "EnemyActions.h"
#include "EnemyIdleState.h"
#include "NeedlePool.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void EnemyShootState::Enter(Enemy& enemy) {
    enemy.PlayAnimation("Attack1", 1.0f, true);
    enemy.StopHorizontal();
    _hasShot = false;

    // 撃つ前にも光らせる 見えないところから針が飛んでこないように
    if (auto* effects = EffectManager::Get()) {
        VECTOR head = VAdd(enemy.GetCenter(), VGet(0.0f, enemy.bodyHeight * 0.5f, 0.0f));
        effects->PlayWarning(head, false);
    }
}

void EnemyShootState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    enemy.FaceTowards(EnemyActions::ToTarget(enemy), enemy.GetData().turnSpeed, deltaTime);

    if (!_hasShot && enemy.GetAnimationTime() >= enemy.GetData().shootTime) {
        _hasShot = true;

        const Character* target = enemy.GetTarget();
        auto* pool = NeedlePool::Get();
        if (target && !target->IsDead() && pool) {
            VECTOR from = VAdd(enemy.GetCenter(), VScale(enemy.GetForward(), 60.0f));
            VECTOR direction = VSub(target->GetCenter(), from);
            pool->Fire(from, direction, enemy.GetData().shootDamage);
        }
    }

    if (enemy.IsAnimationFinished()) {
        enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
    }
}

void EnemyShootState::Exit(Enemy& enemy) {
    enemy.NotifyAttackFinished();
}
