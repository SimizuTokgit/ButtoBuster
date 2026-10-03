#include "EnemyActions.h"
#include "Enemy.h"
#include "EnemyAttackState.h"
#include "EnemyShootState.h"
#include <memory>

using std::make_unique;

VECTOR EnemyActions::ToTarget(const Enemy& enemy) {
    const Character* target = enemy.GetTarget();
    if (!target) return enemy.GetForward();

    VECTOR toTarget = VSub(target->GetPosition(), enemy.GetPosition());
    toTarget.y = 0.0f;
    return toTarget;
}

bool EnemyActions::TryStartAttack(Enemy& enemy, const ICharacterState<Enemy>* from, Technique technique) {
    const EnemyData& data = enemy.GetData();
    auto& states = enemy.GetStates();

    switch (technique) {
    case Technique::Slash: {
        bool isAlt = data.hasSlashAlt && GetRand(1) == 0;
        const AttackData& attack = isAlt ? data.slashAlt : data.slash;
        return states.Transition(from, make_unique<EnemyAttackState>(attack, 0.0f));
    }

    case Technique::StrongSlash:
        if (!data.hasHeavy) return false;
        return states.Transition(from, make_unique<EnemyAttackState>(data.heavy, data.heavyAreaRadius));

    case Technique::Shoot:
        if (!data.canShoot) return false;
        return states.Transition(from, make_unique<EnemyShootState>());

    default:
        return false;
    }
}
