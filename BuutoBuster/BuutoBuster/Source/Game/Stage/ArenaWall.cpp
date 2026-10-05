#include "ArenaWall.h"
#include "StageBuilder.h"
#include "Character.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include "Transform.h"

namespace {
    // ぶつかった強さ 0〜1 FULL_IMPACT_SPEED で 1
    float ImpactPower(const ArenaWall::Hit& hit) {
        float power = hit.speed / ArenaWall::FULL_IMPACT_SPEED;
        return (power > 1.0f) ? 1.0f : power;
    }
}

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
    VECTOR along = VGet(velocity.x - outward.x * speed, 0.0f, velocity.z - outward.z * speed);
    character.SetKnockback(along);

    outHit.point = VGet(
        center.x + outward.x * StageBuilder::ARENA_RADIUS,
        position.y,
        center.z + outward.z * StageBuilder::ARENA_RADIUS);
    outHit.normal = VScale(outward, -1.0f);
    outHit.speed = speed;
    outHit.along = along;
    return true;
}

ArenaWall::Reaction ArenaWall::React(Character& character, const Hit& hit) {
    if (hit.speed < BOUNCE_MIN_SPEED) return Reaction::None;

    // 火花や割れる光は胴の高さに出す
    VECTOR impact = hit.point;
    impact.y = character.GetCenter().y;

    // デバッグの無敵中のように倒されない体は、許容値に届いていても跳ね返す
    if (character.GetBlowRatio() >= 1.0f && character.CanBeDefeated()) {
        // 壁の外へ向けて飛ばす 割った勢いのまま場外へ消えていくように
        float flySpeed = hit.speed * BREAK_FLY_RATE;
        if (flySpeed < BREAK_FLY_MIN_SPEED) flySpeed = BREAK_FLY_MIN_SPEED;
        character.Defeat(VScale(hit.normal, -flySpeed));

        WallBreakEvent event;
        event.character = &character;
        event.isPlayer = character.team == Team::Player;
        event.position = impact;
        event.normal = hit.normal;
        event.speed = hit.speed;
        GetBreakEvents().Notify(event);
        return Reaction::Break;
    }

    // 跳ね返す速さは、張り付いたあとに Bounce で入れる ここではぶつかった手応えだけ出す
    float power = ImpactPower(hit);
    if (auto* effects = EffectManager::Get()) effects->PlayWallHit(impact, hit.normal, power);
    SoundManager::Instance().PlaySE("Golem/downing", 0.5f + 0.5f * power);
    return Reaction::Bounce;
}

float ArenaWall::GetStickTime(const Hit& hit) {
    return STICK_TIME_MIN + (STICK_TIME_MAX - STICK_TIME_MIN) * ImpactPower(hit);
}

void ArenaWall::Bounce(Character& character, const Hit& hit) {
    // 壁に沿う速さは残し、内側への速さを足す 斜めにぶつかれば斜めに跳ね返る
    character.SetKnockback(VAdd(hit.along, VScale(hit.normal, hit.speed * BOUNCE_RATE)));
    character.SetVerticalVelocity(BOUNCE_JUMP_SPEED);
}

Subject<WallBreakEvent>& ArenaWall::GetBreakEvents() {
    // 場面をまたいで 1 つだけ置く 受け取る側は場面ごとに作られ、消えるときに自分から外れる
    static Subject<WallBreakEvent> events;
    return events;
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
