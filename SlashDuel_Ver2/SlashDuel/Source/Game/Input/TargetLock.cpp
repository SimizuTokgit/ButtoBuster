#include "TargetLock.h"
#include "Character.h"
#include "CharacterRegistry.h"
#include <cmath>

namespace {
    VECTOR Flat(VECTOR vector) {
        vector.y = 0.0f;
        return vector;
    }

    // 地面に沿った向きにそろえる 長さが無ければ代わりの向きを使う
    VECTOR FlatDirection(VECTOR vector, VECTOR fallback) {
        vector.y = 0.0f;
        float length = VSize(vector);
        if (length < 0.001f) return fallback;
        return VScale(vector, 1.0f / length);
    }
}

bool TargetLock::Acquire(const Character& owner, VECTOR viewForward) {
    VECTOR view = FlatDirection(viewForward, owner.GetForward());

    const Character* best = nullptr;
    float bestScore = 0.0f;

    for (const Character* other : CharacterRegistry::GetAll()) {
        if (!IsCandidate(owner, other)) continue;

        VECTOR toOther = Flat(VSub(other->GetPosition(), owner.GetPosition()));
        float distance = VSize(toOther);
        if (distance > SEARCH_RANGE) continue;

        // 画面の真ん中に近いほど、近いほど選ばれやすい
        // 真後ろの敵は、同じ距離でも 3 倍遠いとみなす
        float dot = (distance > 0.001f) ? VDot(VScale(toOther, 1.0f / distance), view) : 1.0f;
        float score = distance * (2.0f - dot);

        if (best && score >= bestScore) continue;
        best = other;
        bestScore = score;
    }

    _target = best;
    return best != nullptr;
}

bool TargetLock::Switch(const Character& owner, int direction) {
    if (!_target || direction == 0) return false;

    // ロックオン中のカメラは今の相手を向いているので、画面の右は相手への向きの右になる
    VECTOR base = FlatDirection(VSub(_target->GetPosition(), owner.GetPosition()), owner.GetForward());
    VECTOR right = VCross(VGet(0.0f, 1.0f, 0.0f), base);

    const Character* best = nullptr;
    float bestAngle = 0.0f;

    for (const Character* other : CharacterRegistry::GetAll()) {
        if (other == _target || !IsCandidate(owner, other)) continue;

        VECTOR toOther = Flat(VSub(other->GetPosition(), owner.GetPosition()));
        float distance = VSize(toOther);
        if (distance < 0.001f || distance > SEARCH_RANGE) continue;

        // 今の相手から、指定した側へ何度回ったところにいるか 反対側はマイナスになる
        float angle = atan2f(VDot(toOther, right), VDot(toOther, base)) * 180.0f / DX_PI_F;
        angle *= static_cast<float>(direction);
        if (angle <= 0.0f) continue;

        // 一番近い角度の相手へ移す 飛び越して遠くへ行かないように
        if (best && angle >= bestAngle) continue;
        best = other;
        bestAngle = angle;
    }

    if (!best) return false;
    _target = best;
    return true;
}

void TargetLock::Validate(const Character& owner) {
    if (!_target) return;

    // 消えた相手は中身を読めないので、一覧にいるかだけを先に見る
    if (!CharacterRegistry::Contains(_target) || _target->IsDead()) {
        _target = nullptr;
        return;
    }

    VECTOR toTarget = Flat(VSub(_target->GetPosition(), owner.GetPosition()));
    if (VSize(toTarget) > BREAK_RANGE) _target = nullptr;
}

bool TargetLock::IsCandidate(const Character& owner, const Character* other) {
    if (!other || other == &owner) return false;
    if (other->team == owner.team) return false;
    return !other->IsDead();
}
