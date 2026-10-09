#include "Player.h"
#include "PlayerIdleState.h"
#include "PlayerDamageState.h"
#include "PlayerBlowState.h"
#include "PlayerDeadState.h"
#include "PlayerChargeState.h"
#include "PlayerDodgeState.h"
#include "PlayerRushState.h"
#include "CharacterRegistry.h"
#include "EffectManager.h"
#include "SlashTrail.h"
#include "SoundManager.h"
#include "SkinnedMeshRenderer.h"
#include "GameObject.h"
#include "Transform.h"
#include <memory>

namespace {
    // これより下に落ちたら戻す 地形の穴に落ちたときの保険
    constexpr float FALL_LIMIT_Y = -1500.0f;

    // 頭の上のどこにジャスト回避の知らせを出すか
    constexpr float HEAD_OFFSET = 30.0f;

    // 食らったときの声 吹っ飛ばされ値が許容値のこの割合以上なら、危ないときの声にする
    constexpr const char* DAMAGE_VOICE = "Player/Vc_Damage";
    constexpr const char* DANGER_DAMAGE_VOICE = "Player/Vc_Damage_Danger";
    constexpr float DANGER_VOICE_RATIO = 0.75f;
}

void Player::Start() {
    team = Team::Player;
    _airHangLeft = data.airHangCount;
    _dodgeStock = static_cast<float>(data.dodgeCount);
    _spawnPosition = GetPosition();

    _states.Start(*this, std::make_unique<PlayerIdleState>());
}

void Player::Execute(const InputInfo& input, float deltaTime) {
    UpdateTimers(deltaTime);
    UpdateBlow(deltaTime);
    UpdateWall();
    UpdateCombo(deltaTime);
    UpdateJustDodge(deltaTime);
    UpdateDodgeStock(deltaTime);
    if (_specialLockTimer > 0.0f) _specialLockTimer -= deltaTime;

    // 着地したら、空中で浮き直せる回数を戻す
    if (IsGrounded()) _airHangLeft = data.airHangCount;

    // 回避の残りが無い間に押した回避は、押さなかったことにする
    // 振っている途中に押した回避を覚えておき、残りが戻ったときに出てしまわないように
    InputInfo playerInput = input;
    if (playerInput.technique == Technique::Dodge && !CanDodge()) playerInput.technique = Technique::None;

    // 必殺技も同じ ゲージが満タンでない間に押したものは覚えておかない
    if (playerInput.technique == Technique::Special && !IsSpecialReady()) playerInput.technique = Technique::None;

    _states.Update(*this, playerInput, deltaTime);

    UpdateAnimation(deltaTime);
    ReturnIfFallen();
}

HitResult Player::TakeHit(const HitInfo& info) {
    if (IsDead()) return HitResult::Ignored;

    // 回避を始めた直後に来た攻撃はジャスト回避
    // 回避の無敵より先に見る デバッグの無敵中でも試せるように
    if (_justDodgeTimer > 0.0f) {
        SucceedJustDodge(info.attacker);
        return HitResult::Ignored;
    }

    if (IsInvincible() || isCheatInvincible) return HitResult::Ignored;

    // 敵をゆっくりにしている反撃の間も受けない かわした攻撃の続き (2 回目の判定など) がゆっくり来ても当たらないように
    if (_slowTimer > 0.0f) return HitResult::Ignored;

    auto* effects = EffectManager::Get();

    // 正面から来た攻撃はガードで止める
    bool isFrontal = GetFacingDot(info.sourcePosition) > data.guardDot;
    if (_isGuarding && info.canGuard && isFrontal) {
        _isGuardImpact = true;
        _guardCount++;
        SetKnockback(VScale(info.knockback, data.guardPushRate));

        VECTOR sparkPosition = VAdd(GetCenter(), VScale(GetForward(), 45.0f));
        if (effects) {
            // 盾で弾いた火花は正面へ散らす
            effects->PlayGuard(sparkPosition, GetForward());
            effects->HitStop(0.04f);
        }
        SoundManager::Instance().PlaySE("Player/guard_success");
        SoundManager::Instance().PlaySE("Player/Vc_Guard", 0.8f);

        StaggerGuardedAttacker(info);
        return HitResult::Guarded;
    }

    _hitCount++;
    _combo = 0;
    _comboTimer = 0.0f;

    // 当たった技のダメージの分だけ吹っ飛ばされ値が溜まり、溜まっているほど遠くへ飛ぶ
    AddBlow(static_cast<float>(info.damage));
    VECTOR knockback = ScaleKnockback(info.knockback);

    // 食らったら反撃の機会も失う
    _counterTimer = 0.0f;
    _slowTimer = 0.0f;
    StartFlash();

    if (effects) {
        effects->PlayHit(GetCenter(), knockback);
        effects->Shake(4.0f, 0.2f);
        // 自分を見ていなくても食らったと分かるよう、画面を赤くする
        effects->FlashScreen(0xC02020, 0.3f, 0.25f);
    }
    if (info.hitSound && info.hitSound[0] != '\0') {
        SoundManager::Instance().PlaySE(info.hitSound);
    }

    // 吹っ飛ばされ値が許容値に届いていれば、弱い攻撃でも吹き飛ぶ 壁際で殴られると壁を割られて負ける
    bool isBlowHit = info.reaction == HitReaction::Blow || GetBlowRatio() >= 1.0f;

    // 食らった声 吹き飛ぶときも、のけぞるときも出す
    bool isDanger = GetBlowRatio() >= DANGER_VOICE_RATIO;
    SoundManager::Instance().PlaySE(isDanger ? DANGER_DAMAGE_VOICE : DAMAGE_VOICE);

    auto* current = _states.GetCurrent();
    if (isBlowHit) {
        SoundManager::Instance().PlaySE("Player/blow_B");
        _states.Transition(current, std::make_unique<PlayerBlowState>(knockback));
    }
    else {
        _states.Transition(current, std::make_unique<PlayerDamageState>(knockback));
    }
    return HitResult::Hit;
}

void Player::Defeat(VECTOR knockback) {
    if (IsDead()) return;

    MarkDefeated();
    SoundManager::Instance().PlaySE("Player/Vc_GameOver");
    _states.ForceTransition(std::make_unique<PlayerDeadState>(knockback));
}

void Player::SetTrailEmitting(bool isEmitting) {
    if (_trail) _trail->SetEmitting(isEmitting);
}

bool Player::ConsumeGuardImpact() {
    bool isImpact = _isGuardImpact;
    _isGuardImpact = false;
    return isImpact;
}

bool Player::ConsumeCounter() {
    if (_counterTimer <= 0.0f) return false;

    // 締めを出したら敵の時間も元に戻す 吹き飛ぶところは普段の速さで見せる
    _counterTimer = 0.0f;
    _slowTimer = 0.0f;
    return true;
}

float Player::GetOpponentTimeScale() const {
    return (_slowTimer > 0.0f) ? data.counterSlowScale : 1.0f;
}

float Player::GetOpeningTime() const {
    const auto* current = _states.GetCurrent();
    return current ? current->GetOpeningTime(*this) : 0.0f;
}

bool Player::TryUseAirHang() {
    if (_airHangLeft <= 0) return false;

    _airHangLeft--;
    return true;
}

void Player::UseDodge() {
    _dodgeStock -= 1.0f;
    if (_dodgeStock < 0.0f) _dodgeStock = 0.0f;
}

void Player::AddCombo(int hits) {
    _combo += hits;
    _comboTimer = data.comboKeepTime;
    if (_combo > _maxCombo) _maxCombo = _combo;

    // 当てた数だけ必殺技のゲージが溜まる
    AddSpecialGauge(data.specialGainPerHit * hits);
}

void Player::AddSpecialGauge(float amount) {
    if (amount <= 0.0f || _specialLockTimer > 0.0f) return;

    bool wasReady = IsSpecialReady();
    _specialGauge += amount;
    if (_specialGauge > data.specialGaugeMax) _specialGauge = data.specialGaugeMax;

    // 満タンになった瞬間を 1 回だけ知らせる 撃てるようになったと分かるように
    if (!wasReady && IsSpecialReady()) {
        SpecialEvent event;
        event.type = SpecialEvent::Type::Ready;
        event.position = GetPosition();
        _specialEvents.Notify(event);
    }
}

void Player::FillSpecialGauge() {
    _specialLockTimer = 0.0f;
    AddSpecialGauge(data.specialGaugeMax);
}

void Player::UseSpecial() {
    _specialGauge = 0.0f;
    _specialLockTimer = data.specialRefillDelay;
}

float Player::GetSpecialRatio() const {
    if (data.specialGaugeMax <= 0.0f) return 0.0f;
    return _specialGauge / data.specialGaugeMax;
}

void Player::HealFull() {
    ResetBlow();

    if (auto* effects = EffectManager::Get()) {
        effects->PlayHeal(GetCenter());
        effects->PlayShockwave(GetPosition(), 500.0f, GetColorU8(120, 255, 160, 255));
        effects->FlashScreen(0x60FF90, 0.3f, 0.4f);
    }
    SoundManager::Instance().PlaySE("Game/SE_Recover");
}

VECTOR Player::FindAimDirection(VECTOR inputDirection, float searchRadius) const {
    inputDirection.y = 0.0f;
    float inputLength = VSize(inputDirection);
    VECTOR base = (inputLength > 0.2f) ? VScale(inputDirection, 1.0f / inputLength) : GetForward();

    const Character* best = nullptr;
    float bestDistance = searchRadius;

    for (const Character* other : CharacterRegistry::GetAll()) {
        if (!other || other->team == team || other->IsDead()) continue;

        VECTOR toOther = VSub(other->GetPosition(), GetPosition());
        toOther.y = 0.0f;
        float distance = VSize(toOther);
        if (distance < 0.001f || distance > bestDistance) continue;

        if (VDot(VScale(toOther, 1.0f / distance), base) < data.aimDot) continue;

        best = other;
        bestDistance = distance;
    }

    if (!best) return base;

    VECTOR toBest = VSub(best->GetPosition(), GetPosition());
    toBest.y = 0.0f;
    return toBest;
}

void Player::SetOpacity(float rate) {
    if (_renderer && _renderer->ModelHandle != -1) {
        MV1SetOpacityRate(_renderer->ModelHandle, rate);
    }
}

void Player::UpdateCombo(float deltaTime) {
    if (_comboTimer <= 0.0f) return;

    _comboTimer -= deltaTime;
    if (_comboTimer <= 0.0f) {
        _comboTimer = 0.0f;
        _combo = 0;
    }
}

void Player::UpdateJustDodge(float deltaTime) {
    if (_justDodgeTimer > 0.0f) _justDodgeTimer -= deltaTime;

    // 溜めている間は反撃の残りを減らさない ジャスト回避から溜めて振っても反撃になるように
    if (_counterTimer > 0.0f && !_states.IsIn<PlayerChargeState>()) _counterTimer -= deltaTime;

    // 敵をゆっくりにしておく時間は溜めている間も減らす 溜め続けて敵を止めておけないように
    if (_slowTimer > 0.0f) _slowTimer -= deltaTime;
}

void Player::UpdateDodgeStock(float deltaTime) {
    // 回避している間は戻さない 続けて使い切ったら、止まってから戻り始める
    if (_states.IsIn<PlayerDodgeState>()) return;

    float fullStock = static_cast<float>(data.dodgeCount);

    // 戻る時間を 0 にしたら、すぐ戻る
    if (data.dodgeRechargeTime <= 0.0f) {
        _dodgeStock = fullStock;
        return;
    }

    _dodgeStock += deltaTime / data.dodgeRechargeTime;
    if (_dodgeStock > fullStock) _dodgeStock = fullStock;
}

void Player::StaggerGuardedAttacker(const HitInfo& info) {
    // 針のように撃った本人がいない攻撃と、飛んできた敵がぶつかった攻撃は弾き返さない
    if (!info.attacker || info.isFromProjectile) return;

    // 自分から相手へ向かう向きに押し返す
    VECTOR away = VSub(info.attacker->GetPosition(), GetPosition());
    away.y = 0.0f;
    float distance = VSize(away);
    if (distance < 0.001f) return;

    VECTOR pushBack = VScale(away, data.guardStaggerPush / distance);
    info.attacker->OnAttackGuarded(pushBack);
}

void Player::SucceedJustDodge(const Character* attacker) {
    _justDodgeTimer = 0.0f;
    _counterTimer = data.counterTime;
    _slowTimer = data.counterTime;
    SetInvincible(data.justDodgeInvincibleTime);

    // かわした相手の目の前まで自動で寄る 針のように、誰の攻撃か分からないときは寄らない
    if (attacker && !attacker->IsDead()) {
        _states.Transition(_states.GetCurrent(),
            std::make_unique<PlayerRushState>(attacker->GetPosition(), attacker->bodyRadius));
    }

    // 画面の演出 音 画面の文字は、知らせを受け取った側が出す
    JustDodgeEvent event;
    event.position = GetPosition();
    event.headPosition = VAdd(event.position, VGet(0.0f, bodyHeight + HEAD_OFFSET, 0.0f));
    _justDodgeEvents.Notify(event);
}

void Player::ReturnIfFallen() {
    // 場外へ飛ばされたあとは戻さない
    if (IsDead()) return;

    VECTOR position = transform->localPosition;
    if (position.y >= FALL_LIMIT_Y) return;

    transform->localPosition = _spawnPosition;
    SetVerticalVelocity(0.0f);
}

bool Player::IsCharging() const {
    return _states.IsIn<PlayerChargeState>();
}
