#include "Enemy.h"
#include "EnemyIdleState.h"
#include "EnemyDamageState.h"
#include "EnemyBlowState.h"
#include "EnemyDeadState.h"
#include "EffectManager.h"
#include "StageBuilder.h"
#include "SoundManager.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "SkinnedMeshRenderer.h"
#include <cmath>
#include <memory>

namespace {
    // これより下に落ちたら、地形の穴に落ちたとみなして片付ける
    constexpr float FALL_LIMIT_Y = -1500.0f;

    // 浮いている高さに戻ろうとする強さ
    constexpr float HOVER_STIFFNESS = 4.0f;
    constexpr float HOVER_MAX_SPEED = 500.0f;

    // 羽ばたきに合わせて上下に揺らす
    constexpr float HOVER_BOB_HEIGHT = 20.0f;
    constexpr float HOVER_BOB_SPEED = 3.0f;
}

void Enemy::Initialize(const EnemyData& data, int id, Character* target) {
    _data = &data;
    _id = id;
    _target = target;

    team = Team::Enemy;
    maxHp = data.maxHp;
    hp = data.maxHp;
    weight = data.weight;
}

void Enemy::Start() {
    SetHovering(_data->isFlying);
    _states.Start(*this, std::make_unique<EnemyIdleState>());
}

void Enemy::Execute(const InputInfo& input, float deltaTime) {
    UpdateTimers(deltaTime);
    if (_hpBarTimer > 0.0f) _hpBarTimer -= deltaTime;

    _states.Update(*this, input, deltaTime);

    if (_isHovering) KeepHovering(deltaTime);
    UpdateAnimation(deltaTime);

    if (GetPosition().y < FALL_LIMIT_Y) {
        hp = 0;
        MarkReadyToRemove();
    }
}

HitResult Enemy::TakeHit(const HitInfo& info) {
    if (IsDead()) return HitResult::Ignored;

    hp -= info.damage;
    _hpBarTimer = HP_BAR_TIME;
    StartFlash();

    auto* effects = EffectManager::Get();
    if (effects) effects->PlayHit(GetCenter(), info.knockback);

    // 飛んできた敵がぶつかった音は、連鎖の知らせを受けた側が鳴らす 斬られた音とは違うので
    if (!info.isFromProjectile) SoundManager::Instance().PlaySE(_data->soundHit, 0.8f);

    // 飛んできた敵に当たったときは、重くて剣では止まらない敵も吹き飛ぶ
    bool isBlown = _data->canBlow || info.isFromProjectile;

    if (hp <= 0) {
        hp = 0;
        if (effects) effects->PlayKill(GetCenter(), info.knockback);
        SoundManager::Instance().PlaySE(_data->soundDead);
        _states.ForceTransition(std::make_unique<EnemyDeadState>(info.knockback, info.chain, isBlown));
        return HitResult::Killed;
    }

    // Golem は殴っても止まらない
    if (!_data->canFlinch && !info.isFromProjectile) return HitResult::Hit;

    auto* current = _states.GetCurrent();
    if (info.reaction == HitReaction::Blow && isBlown) {
        SoundManager::Instance().PlaySE(_data->soundBlow);
        _states.Transition(current, std::make_unique<EnemyBlowState>(info.knockback, info.chain));
    }
    else {
        SoundManager::Instance().PlaySE(_data->soundDamage, 0.7f);
        _states.Transition(current, std::make_unique<EnemyDamageState>(info.knockback));
    }
    return HitResult::Hit;
}

bool Enemy::ConsumeAttackFinished() {
    bool isFinished = _hasFinishedAttack;
    _hasFinishedAttack = false;
    return isFinished;
}

void Enemy::SetHovering(bool isHovering) {
    _isHovering = isHovering;
    SetGravityEnabled(!isHovering);
}

bool Enemy::TryCountDefeat() {
    if (_isCounted || !IsDead()) return false;
    _isCounted = true;
    return true;
}

void Enemy::SetOpacity(float rate) {
    if (_renderer && _renderer->ModelHandle != -1) {
        MV1SetOpacityRate(_renderer->ModelHandle, rate);
    }

    // 持っている武器も一緒に消す
    for (auto* mesh : gameObject->GetComponentsInChildren<MeshRenderer>()) {
        if (mesh->ModelHandle != -1) MV1SetOpacityRate(mesh->ModelHandle, rate);
    }
}

void Enemy::KeepHovering(float deltaTime) {
    VECTOR position = GetPosition();

    // 地面が見つからなければ今の高さを保つ
    float groundY = position.y - _data->hoverHeight;
    StageBuilder::FindGroundHeight(position.x, position.z, groundY);

    // 敵ごとに揺れの時間をずらし、群れが同じ動きで上下しないようにする
    float phase = GetNowCount() / 1000.0f * HOVER_BOB_SPEED + _id;
    float targetY = groundY + _data->hoverHeight + sinf(phase) * HOVER_BOB_HEIGHT;

    float speed = (targetY - position.y) * HOVER_STIFFNESS;
    if (speed > HOVER_MAX_SPEED) speed = HOVER_MAX_SPEED;
    if (speed < -HOVER_MAX_SPEED) speed = -HOVER_MAX_SPEED;
    SetVerticalVelocity(speed);
}
