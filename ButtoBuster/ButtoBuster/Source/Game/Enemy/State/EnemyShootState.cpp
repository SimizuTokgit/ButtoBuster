#include "EnemyShootState.h"
#include "Enemy.h"
#include "EnemyActions.h"
#include "EnemyIdleState.h"
#include "NeedlePool.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

namespace {
    // 針を出す所 胴体の真ん中から、正面へこの距離
    constexpr float MUZZLE_FORWARD = 60.0f;
}

void EnemyShootState::Enter(Enemy& enemy) {
    enemy.PlayAnimation("Attack1", 1.0f, true);
    enemy.StopHorizontal();
    _hasShot = false;

    // 撃つ前にも光らせる 見えないところから針が飛んでこないように
    if (auto* effects = EffectManager::Get()) effects->PlayWarning(enemy.GetTopPosition(), false);
}

void EnemyShootState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    const EnemyData& data = enemy.GetData();
    enemy.FaceTowards(EnemyActions::ToTarget(enemy), data.turnSpeed, deltaTime);

    if (!_hasShot && enemy.GetAnimationTime() >= data.shootTime) {
        _hasShot = true;

        const Character* target = enemy.GetTarget();
        auto* pool = NeedlePool::Get();
        if (target && !target->IsDead() && pool) {
            VECTOR from = VAdd(enemy.GetCenter(), VScale(enemy.GetForward(), MUZZLE_FORWARD));
            VECTOR direction = VSub(target->GetCenter(), from);

            // 針が刺さった音は、急降下して刺したときと同じ
            pool->Fire(from, direction, data.shootDamage, data.slash.hitSound);
        }
    }

    if (enemy.IsAnimationFinished()) {
        enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
    }
}

void EnemyShootState::Exit(Enemy& enemy) {
    enemy.NotifyAttackFinished();
}
