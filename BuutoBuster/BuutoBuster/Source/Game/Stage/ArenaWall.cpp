#include "ArenaWall.h"
#include "StageBuilder.h"
#include "Character.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include "Transform.h"

bool ArenaWall::KeepInside(Character& character, Hit& outHit) {
    VECTOR center = StageBuilder::GetArenaCenter();
    VECTOR position = character.transform->localPosition;

    VECTOR offset = VSub(position, center);
    offset.y = 0.0f;
    float distance = VSize(offset);

    // 体の太さの分だけ手前で止める 体が光の幕にめり込んで見えないように
    float limit = StageBuilder::ARENA_RADIUS - character.bodyRadius;
    if (distance <= limit) return false;

    VECTOR outward = VScale(offset, 1.0f / distance);
    position.x = center.x + outward.x * limit;
    position.z = center.z + outward.z * limit;
    character.transform->localPosition = position;

    VECTOR velocity = character.GetVelocity();
    float speed = velocity.x * outward.x + velocity.z * outward.z;
    if (speed <= 0.0f) return false;

    // 壁へ向かう分の速さだけ消す 壁に沿う分は残すので、斜めにぶつかると壁沿いに滑る
    character.SetKnockback(VGet(velocity.x - outward.x * speed, 0.0f, velocity.z - outward.z * speed));

    outHit.point = VGet(
        center.x + outward.x * StageBuilder::ARENA_RADIUS,
        position.y,
        center.z + outward.z * StageBuilder::ARENA_RADIUS);
    outHit.normal = VScale(outward, -1.0f);
    outHit.speed = speed;
    return true;
}

bool ArenaWall::TryBounce(Character& character, const Hit& hit) {
    if (hit.speed < BOUNCE_MIN_SPEED) return false;

    // 壁へ向かう分は KeepInside で消してあるので、内側への速さを足せば跳ね返る
    VECTOR velocity = VAdd(character.GetVelocity(), VScale(hit.normal, hit.speed * BOUNCE_RATE));
    character.SetKnockback(velocity);
    if (velocity.y < BOUNCE_JUMP_SPEED) character.SetVerticalVelocity(BOUNCE_JUMP_SPEED);

    float power = hit.speed / FULL_IMPACT_SPEED;
    if (power > 1.0f) power = 1.0f;

    // 火花は胴の高さに出す
    VECTOR impact = hit.point;
    impact.y = character.GetCenter().y;
    if (auto* effects = EffectManager::Get()) effects->PlayWallHit(impact, hit.normal, power);
    SoundManager::Instance().PlaySE("Golem/downing", 0.5f + 0.5f * power);
    return true;
}

VECTOR ArenaWall::ClampInside(VECTOR position, float margin) {
    VECTOR center = StageBuilder::GetArenaCenter();
    VECTOR offset = VSub(position, center);
    offset.y = 0.0f;
    float distance = VSize(offset);

    float limit = StageBuilder::ARENA_RADIUS - margin;
    if (distance <= limit) return position;

    position.x = center.x + offset.x * (limit / distance);
    position.z = center.z + offset.z * (limit / distance);
    return position;
}
