#include "Character.h"
#include "CharacterRegistry.h"
#include "GameObject.h"
#include "Transform.h"
#include "Animator.h"
#include "SkinnedMeshRenderer.h"
#include "Rigidbody.h"
#include "CapsuleCollider.h"
#include <cmath>

namespace {
    float YawFromDirection(VECTOR direction) {
        return Transform::Rad2Deg(atan2f(direction.x, direction.z));
    }

    float WrapDegree(float degree) {
        while (degree > 180.0f) degree -= 360.0f;
        while (degree < -180.0f) degree += 360.0f;
        return degree;
    }
}

Character::~Character() {
    CharacterRegistry::Remove(this);
}

void Character::Setup(Animator* animator, SkinnedMeshRenderer* renderer, Rigidbody* rigidbody, CapsuleCollider* body) {
    _animator = animator;
    _renderer = renderer;
    _rigidbody = rigidbody;
    _body = body;

    if (_body) {
        bodyRadius = _body->radius;
        bodyHeight = _body->height;
    }

    CharacterRegistry::Add(this);
}

void Character::SetInvincible(float seconds) {
    if (seconds > _invincibleTimer) _invincibleTimer = seconds;
}

void Character::PlayAnimation(const std::string& name, float speed, bool restart) {
    _animationSpeed = speed;

    bool isSame = _animator && _animator->GetCurrentAnimationName() == name;
    _animationName = name;
    if (!_animator) return;

    if (isSame) {
        if (restart) _animator->Rewind();
        return;
    }

    // その場で付け替えて時間を 0 にしておく
    // 次の Play まで待つと、同じフレームの判定が前のアニメの時間を読んでしまう
    _animator->Play(name, 0.0f);
}

float Character::GetAnimationTime() const {
    return _animator ? _animator->GetCurrentTime() : 0.0f;
}

void Character::SetAnimationTime(float time) {
    if (_animator) _animator->SetCurrentTime(time);
}

bool Character::IsAnimationFinished() const {
    return _animator ? _animator->IsFinished() : true;
}

void Character::SetHorizontalVelocity(VECTOR direction, float speed) {
    if (!_rigidbody) return;

    direction.y = 0.0f;
    float length = VSize(direction);

    // スティックを半分倒したら半分の速さ 1 を超えたときだけ縮める
    if (length > 1.0f) direction = VScale(direction, 1.0f / length);

    _rigidbody->linearVelocity.x = direction.x * speed;
    _rigidbody->linearVelocity.z = direction.z * speed;
}

void Character::StopHorizontal() {
    if (!_rigidbody) return;
    _rigidbody->linearVelocity.x = 0.0f;
    _rigidbody->linearVelocity.z = 0.0f;
}

void Character::SetKnockback(VECTOR velocity) {
    if (!_rigidbody) return;
    _rigidbody->linearVelocity.x = velocity.x;
    _rigidbody->linearVelocity.z = velocity.z;
}

void Character::DampHorizontal(float rate, float deltaTime) {
    if (!_rigidbody) return;

    float keep = 1.0f - rate * deltaTime;
    if (keep < 0.0f) keep = 0.0f;
    _rigidbody->linearVelocity.x *= keep;
    _rigidbody->linearVelocity.z *= keep;
}

void Character::SetVerticalVelocity(float speed) {
    if (!_rigidbody) return;
    _rigidbody->linearVelocity.y = speed;
}

void Character::SetGravityEnabled(bool isEnabled) {
    if (!_rigidbody) return;
    _rigidbody->useGravity = isEnabled;
}

void Character::SetKinematic(bool isKinematic) {
    if (!_rigidbody) return;
    _rigidbody->isKinematic = isKinematic;
    if (isKinematic) _rigidbody->linearVelocity = VGet(0.0f, 0.0f, 0.0f);
}

void Character::SetBodySolid(bool isSolid) {
    if (_body) _body->isTrigger = !isSolid;
}

VECTOR Character::GetVelocity() const {
    return _rigidbody ? _rigidbody->linearVelocity : VGet(0.0f, 0.0f, 0.0f);
}

bool Character::IsGrounded() const {
    return _rigidbody ? _rigidbody->isGrounded : true;
}

bool Character::ConsumeWallHit(ArenaWall::Hit& outHit) {
    if (!_hasWallHit) return false;

    _hasWallHit = false;
    outHit = _wallHit;
    return true;
}

void Character::FaceTowards(VECTOR direction, float degreesPerSecond, float deltaTime) {
    direction.y = 0.0f;
    if (VSquareSize(direction) < 0.0001f) return;

    float current = YawFromDirection(GetForward());
    float target = YawFromDirection(direction);
    float difference = WrapDegree(target - current);
    float step = degreesPerSecond * deltaTime;

    if (fabsf(difference) <= step) {
        current = target;
    }
    else {
        current += (difference > 0.0f) ? step : -step;
    }

    transform->localRotation = Quaternion::Euler(0.0f, current, 0.0f);
}

void Character::FaceImmediately(VECTOR direction) {
    direction.y = 0.0f;
    if (VSquareSize(direction) < 0.0001f) return;

    transform->localRotation = Quaternion::Euler(0.0f, YawFromDirection(direction), 0.0f);
}

VECTOR Character::GetForward() const {
    VECTOR forward = transform->forward;
    forward.y = 0.0f;

    float length = VSize(forward);
    if (length < 0.0001f) return VGet(0.0f, 0.0f, 1.0f);
    return VScale(forward, 1.0f / length);
}

float Character::GetFacingDot(VECTOR worldPosition) const {
    VECTOR toTarget = VSub(worldPosition, GetPosition());
    toTarget.y = 0.0f;

    float length = VSize(toTarget);
    if (length < 0.0001f) return 1.0f;
    return VDot(VScale(toTarget, 1.0f / length), GetForward());
}

VECTOR Character::GetPosition() const {
    return transform->position;
}

VECTOR Character::GetCenter() const {
    if (_body) return _body->GetWorldCenter();
    return VAdd(GetPosition(), VGet(0.0f, bodyHeight * 0.5f, 0.0f));
}

bool Character::IsTouchingSphere(VECTOR center, float radius) const {
    if (!_body) {
        VECTOR toCenter = VSub(center, GetCenter());
        return VSize(toCenter) < radius + bodyRadius;
    }

    VECTOR bottom;
    VECTOR top;
    _body->GetCapsulePoints(bottom, top);
    return HitCheck_Sphere_Capsule(center, radius, bottom, top, _body->radius) == TRUE;
}

void Character::UpdateTimers(float deltaTime) {
    if (_invincibleTimer > 0.0f) _invincibleTimer -= deltaTime;
    if (_flashTimer > 0.0f) _flashTimer -= deltaTime;
    UpdateFlash();
}

void Character::UpdateAnimation(float deltaTime) {
    if (!_animator || _animationName.empty()) return;
    _animator->Play(_animationName, deltaTime * _animationSpeed);
}

void Character::UpdateWall() {
    _hasWallHit = ArenaWall::KeepInside(*this, _wallHit);
}

void Character::UpdateFlash() {
    if (!_renderer || _renderer->ModelHandle == -1) return;

    // 1 より大きくすると明るくなる 当たった瞬間だけ白っぽく光らせる
    float rate = (_flashTimer > 0.0f) ? _flashTimer / FLASH_TIME : 0.0f;
    float scale = 1.0f + rate * 2.0f;
    MV1SetDifColorScale(_renderer->ModelHandle, GetColorF(scale, scale, scale, 1.0f));
}
