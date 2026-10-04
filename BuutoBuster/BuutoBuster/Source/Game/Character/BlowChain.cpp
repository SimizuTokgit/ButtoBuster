#include "BlowChain.h"
#include "Character.h"
#include "CharacterRegistry.h"
#include "HitInfo.h"
#include <algorithm>
#include <cmath>

namespace {
    VECTOR Flatten(VECTOR vector) {
        vector.y = 0.0f;
        return vector;
    }

    // 点から線分までの距離の 2 乗
    // std::min と std::max は Windows.h のマクロとぶつかるので比べて書く
    float SquareDistanceToSegment(VECTOR point, VECTOR start, VECTOR end) {
        VECTOR segment = VSub(end, start);
        float lengthSquare = VSquareSize(segment);

        float t = 0.0f;
        if (lengthSquare > 0.0001f) {
            t = VDot(VSub(point, start), segment) / lengthSquare;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }

        VECTOR closest = VAdd(start, VScale(segment, t));
        return VSquareSize(VSub(point, closest));
    }
}

std::shared_ptr<BlowChain> BlowChain::Create(Subject<ChainEvent>* events) {
    return std::make_shared<BlowChain>(events);
}

void BlowChain::BeginFlight(const Character& projectile) {
    if (!Contains(&projectile)) _members.push_back(&projectile);
    _flyingCount++;

    // 一度全員が止まったあとに、同じ振りでまた飛んだときは続きとして数える
    _hasEnded = false;
}

void BlowChain::EndFlight(VECTOR position) {
    if (_flyingCount > 0) _flyingCount--;
    if (_flyingCount > 0 || _hasEnded) return;

    _hasEnded = true;
    Notify(ChainEvent::Type::End, position);
}

bool BlowChain::Sweep(Character& projectile, float deltaTime) {
    VECTOR velocity = Flatten(projectile.GetVelocity());
    float speed = VSize(velocity);
    if (speed < MIN_SPEED) return false;

    VECTOR direction = VScale(velocity, 1.0f / speed);

    // 今の位置だけでなく、このフレームに進んだ線で見る 速い砲弾が相手をすり抜けないように
    VECTOR end = projectile.GetCenter();
    VECTOR start = VSub(end, VScale(velocity, deltaTime));

    for (Character* target : CharacterRegistry::GetAll()) {
        if (!target || target == &projectile) continue;

        // 巻き込むのは同じ側の敵だけ 自分で飛ばした敵がプレイヤーに当たらないように
        if (target->team != projectile.team) continue;
        if (target->IsDead() || Contains(target)) continue;

        VECTOR center = target->GetCenter();
        float reach = projectile.bodyRadius + target->bodyRadius + TOUCH_MARGIN;
        if (SquareDistanceToSegment(Flatten(center), Flatten(start), Flatten(end)) > reach * reach) continue;

        float heightGap = fabsf(center.y - end.y);
        if (heightGap > (projectile.bodyHeight + target->bodyHeight) * 0.5f) continue;

        float weightRate = (target->weight > 0.0f) ? projectile.weight / target->weight : 1.0f;
        if (weightRate < WEIGHT_RATE_MIN) weightRate = WEIGHT_RATE_MIN;
        if (weightRate > WEIGHT_RATE_MAX) weightRate = WEIGHT_RATE_MAX;

        // 進む向きと、砲弾から相手への向きの間へ飛ばす 群れがボウリングのピンのように散る
        VECTOR pushDirection = direction;
        VECTOR toTarget = Flatten(VSub(center, end));
        float toTargetLength = VSize(toTarget);
        if (toTargetLength > 0.001f) {
            VECTOR blended = VAdd(direction, VScale(toTarget, 1.0f / toTargetLength));
            float blendedLength = VSize(blended);
            if (blendedLength > 0.001f) pushDirection = VScale(blended, 1.0f / blendedLength);
        }

        // 先に入れておく 同じフレームにほかの砲弾が同じ相手に当たって、二重に数えないように
        _members.push_back(target);

        HitInfo info;
        info.attacker = &projectile;
        info.sourcePosition = projectile.GetPosition();
        info.damage = static_cast<int>(BASE_DAMAGE + speed * SPEED_DAMAGE_RATE + (_hitCount + 1) * COUNT_DAMAGE);
        info.reaction = HitReaction::Blow;
        info.knockback = VScale(pushDirection, speed * TRANSFER_RATE * weightRate);
        info.canGuard = false;
        info.chain = shared_from_this();
        info.isFromProjectile = true;

        if (target->TakeHit(info) == HitResult::Ignored) continue;

        _hitCount++;
        Notify(ChainEvent::Type::Hit, VScale(VAdd(center, end), 0.5f));

        // 当てた分だけ勢いを失い、そのまま飛び続ける
        velocity = VScale(velocity, KEEP_RATE);
        speed *= KEEP_RATE;
        projectile.SetKnockback(velocity);
        if (speed < MIN_SPEED) return false;
    }
    return true;
}

bool BlowChain::Contains(const Character* character) const {
    return std::find(_members.begin(), _members.end(), character) != _members.end();
}

void BlowChain::Notify(ChainEvent::Type type, VECTOR position) {
    if (!_events) return;

    ChainEvent event;
    event.type = type;
    event.count = _hitCount;
    event.position = position;
    _events->Notify(event);
}
