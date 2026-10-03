#include "Needle.h"
#include "Character.h"
#include "CharacterRegistry.h"
#include "StageBuilder.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"

void Needle::Launch(VECTOR position, VECTOR direction, int damage) {
    float length = VSize(direction);
    if (length < 0.001f) return;

    VECTOR forward = VScale(direction, 1.0f / length);
    _velocity = VScale(forward, SPEED);
    _life = LIFE_TIME;
    _damage = damage;
    _isFlying = true;

    transform->localPosition = position;

    // 針のモデルは +X に伸びているので、+X を飛ぶ向きに合わせる
    // Y 軸で -90 度回すと +X が +Z を向き、そこから +Z を飛ぶ向きへ回す
    transform->localRotation = Quaternion::LookRotation(forward) * Quaternion::Euler(0.0f, -90.0f, 0.0f);

    if (_renderer) _renderer->enabled = true;
}

void Needle::Deactivate() {
    _isFlying = false;
    if (_renderer) _renderer->enabled = false;
}

void Needle::Update(float deltaTime) {
    if (!_isFlying) return;

    _life -= deltaTime;
    if (_life <= 0.0f) {
        Deactivate();
        return;
    }

    VECTOR next = VAdd(transform->localPosition, VScale(_velocity, deltaTime));
    transform->localPosition = next;

    // 地面に刺さったら消える
    float groundY = 0.0f;
    if (StageBuilder::FindGroundHeight(next.x, next.z, groundY) && next.y < groundY) {
        Deactivate();
        return;
    }

    if (TryHitPlayer(next)) Deactivate();
}

bool Needle::TryHitPlayer(VECTOR position) {
    for (Character* character : CharacterRegistry::GetAll()) {
        if (!character || character->team != Team::Player || character->IsDead()) continue;
        if (!character->IsTouchingSphere(position, HIT_RADIUS)) continue;

        VECTOR flat = VGet(_velocity.x, 0.0f, _velocity.z);
        float flatLength = VSize(flat);
        VECTOR push = (flatLength > 0.001f) ? VScale(flat, KNOCKBACK / flatLength) : VGet(0.0f, 0.0f, 0.0f);

        HitInfo info;
        // 撃った Bee はもう倒れているかもしれないので、針が来た方向を向きの判定に使う
        info.sourcePosition = VSub(position, VScale(_velocity, 0.2f));
        info.damage = _damage;
        info.reaction = HitReaction::Flinch;
        info.knockback = push;
        info.canGuard = true;
        info.hitSound = "Player/dmg_byNeedle";

        character->TakeHit(info);
        return true;
    }
    return false;
}
