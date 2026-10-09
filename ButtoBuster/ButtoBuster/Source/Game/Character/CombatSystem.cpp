#include "CombatSystem.h"
#include "Character.h"
#include "CharacterRegistry.h"
#include "Transform.h"
#include <algorithm>
#include <cmath>

namespace {
    bool IsTarget(const Character& attacker, const Character* target, const std::vector<Character*>& hitList) {
        if (!target || target == &attacker) return false;
        if (target->team == attacker.team) return false;
        if (target->IsDead()) return false;
        return std::find(hitList.begin(), hitList.end(), target) == hitList.end();
    }

    // 相手の体の上下の幅が、攻撃の届く高さと重なっているか
    bool IsInHeightRange(const Character& attacker, const Character& target, const AttackData& attack) {
        float baseY = attacker.GetPosition().y;
        float bottom = target.GetCenter().y - target.bodyHeight * 0.5f;
        float top = bottom + target.bodyHeight;
        return top >= baseY + attack.heightMin && bottom <= baseY + attack.heightMax;
    }

    bool Hit(Character& attacker, Character& target, const AttackData& attack, VECTOR toTarget,
        const std::shared_ptr<BlowChain>& chain) {
        toTarget.y = 0.0f;
        float length = VSize(toTarget);
        VECTOR direction = (length > 0.001f) ? VScale(toTarget, 1.0f / length) : attacker.GetForward();

        HitInfo info;
        info.attacker = &attacker;
        info.sourcePosition = attacker.GetPosition();
        info.damage = attack.damage;
        info.reaction = attack.reaction;
        info.knockback = VScale(direction, attack.knockback);
        info.lift = attack.lift;
        info.canGuard = attack.canGuard;
        info.hitSound = attack.hitSound;
        info.chain = chain;

        return target.TakeHit(info) != HitResult::Ignored;
    }
}

int CombatSystem::ApplyMelee(Character& attacker, const AttackData& attack, std::vector<Character*>& hitList,
    const std::shared_ptr<BlowChain>& chain) {
    VECTOR origin = attacker.GetPosition();
    VECTOR forward = attacker.GetForward();
    float cosArc = cosf(Transform::Deg2Rad(attack.arcDegree));

    int count = 0;
    for (Character* target : CharacterRegistry::GetAll()) {
        if (!IsTarget(attacker, target, hitList)) continue;

        VECTOR toTarget = VSub(target->GetPosition(), origin);
        toTarget.y = 0.0f;
        float distance = VSize(toTarget);
        if (distance > attack.reach + target->bodyRadius) continue;

        // 体が触れるほど近いなら向きは問わない 足元の敵を空振りしないように
        bool isTouching = distance < attacker.bodyRadius + target->bodyRadius + 20.0f;
        if (!isTouching) {
            float dot = VDot(VScale(toTarget, 1.0f / distance), forward);
            if (dot < cosArc) continue;
        }

        if (!IsInHeightRange(attacker, *target, attack)) continue;

        hitList.push_back(target);
        if (Hit(attacker, *target, attack, toTarget, chain)) count++;
    }
    return count;
}

int CombatSystem::ApplyArea(Character& attacker, float radius, const AttackData& attack, std::vector<Character*>& hitList,
    const std::shared_ptr<BlowChain>& chain) {
    VECTOR origin = attacker.GetPosition();

    int count = 0;
    for (Character* target : CharacterRegistry::GetAll()) {
        if (!IsTarget(attacker, target, hitList)) continue;

        VECTOR toTarget = VSub(target->GetPosition(), origin);
        toTarget.y = 0.0f;
        if (VSize(toTarget) > radius + target->bodyRadius) continue;
        if (!IsInHeightRange(attacker, *target, attack)) continue;

        hitList.push_back(target);
        if (Hit(attacker, *target, attack, toTarget, chain)) count++;
    }
    return count;
}

int CombatSystem::ApplyAll(Character& attacker, const AttackData& attack, std::vector<Character*>& hitList,
    const std::shared_ptr<BlowChain>& chain) {
    VECTOR origin = attacker.GetPosition();

    int count = 0;
    for (Character* target : CharacterRegistry::GetAll()) {
        if (!IsTarget(attacker, target, hitList)) continue;

        hitList.push_back(target);
        if (Hit(attacker, *target, attack, VSub(target->GetPosition(), origin), chain)) count++;
    }
    return count;
}
