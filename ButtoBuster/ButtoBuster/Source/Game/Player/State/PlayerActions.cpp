#include "PlayerActions.h"
#include "Player.h"
#include "PlayerAirSlamState.h"
#include "PlayerAttacks.h"
#include "PlayerAttackState.h"
#include "PlayerChargeState.h"
#include "PlayerDodgeState.h"
#include "PlayerGuardState.h"
#include "PlayerJumpState.h"
#include "PlayerSpecialState.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

namespace {
    // 寄る技が当たったときの手応え 寄る角度は技 (AttackData の zoomPunch) から読む
    struct ZoomFlashLook {
        // 寄る長さ 秒
        float zoomTime = 0.35f;

        // 画面を光らせる色 (0xRRGGBB) と、寄る角度 1 度あたりの濃さと、光る長さ 秒
        unsigned int flashColor = 0xFFFFFF;
        float flashAlphaPerDegree = 0.04f;
        float flashTime = 0.12f;
    };
    constexpr ZoomFlashLook ZOOM_FLASH{};
}

bool PlayerActions::HasMoveInput(const InputInfo& input) {
    return VSize(input.move) > MOVE_INPUT_THRESHOLD;
}

bool PlayerActions::TryStart(Player& player, const ICharacterState<Player>* from, const InputInfo& input) {
    auto& states = player.GetStates();

    switch (input.technique) {
    case Technique::Slash:
        return states.Transition(from,
            make_unique<PlayerAttackState>(PlayerAttacks::GetSlash(0), 0, PlayerAttackState::Kind::Ground));

    // 強攻撃はヘビーアタック 押し続けると溜まり、すぐ離せば溜めずに振る
    case Technique::StrongSlash:
        return states.Transition(from, make_unique<PlayerChargeState>());

    case Technique::AntiAir:
        return states.Transition(from,
            make_unique<PlayerAttackState>(PlayerAttacks::GetAntiAir(), -1, PlayerAttackState::Kind::AntiAir));

    case Technique::Dodge: {
        // 回避の残りが無ければ出さない ガードを握っていれば、そのままガードになる
        if (!player.CanDodge()) return false;

        // 倒していなければ後ろへ下がる
        VECTOR direction = HasMoveInput(input) ? input.move : VScale(player.GetForward(), -1.0f);
        return states.Transition(from, make_unique<PlayerDodgeState>(direction));
    }

    case Technique::Jump:
        if (!player.IsGrounded()) return false;
        return states.Transition(from, make_unique<PlayerJumpState>(true));

    // 必殺技はゲージが満タンのときだけ 地上でだけ出せる (空中で回避したあとに押しても出ない)
    case Technique::Special:
        if (!player.IsSpecialReady() || !player.IsGrounded()) return false;
        return states.Transition(from, make_unique<PlayerSpecialState>());

    default:
        break;
    }

    if (input.isGuardHeld) {
        return states.Transition(from, make_unique<PlayerGuardState>());
    }

    return false;
}

bool PlayerActions::TryStartAir(Player& player, const ICharacterState<Player>* from, const InputInfo& input) {
    auto& states = player.GetStates();

    switch (input.technique) {
    case Technique::Slash:
        return states.Transition(from,
            make_unique<PlayerAttackState>(PlayerAttacks::GetAirSlash(0), 0, PlayerAttackState::Kind::Air));

    // 空中では溜めずに、すぐ真下へ叩きつける
    // 跳んだ直後でまだ地面から離れていないうちは出さない その場での叩きつけになってしまうので
    case Technique::StrongSlash:
        if (player.IsGrounded()) return false;
        return states.Transition(from, make_unique<PlayerAirSlamState>());

    // 空の Bee を落とすための斬り上げは、空中でも出せる
    case Technique::AntiAir:
        return states.Transition(from,
            make_unique<PlayerAttackState>(PlayerAttacks::GetAntiAir(), -1, PlayerAttackState::Kind::AntiAir));

    default:
        return false;
    }
}

void PlayerActions::QueuedTechnique::Remember(const InputInfo& input) {
    if (input.technique != Technique::None) technique = input.technique;
}

InputInfo PlayerActions::QueuedTechnique::ToInput(const InputInfo& input) const {
    InputInfo queued = input;
    queued.technique = technique;
    return queued;
}

bool PlayerActions::QueuedTechnique::TryStart(Player& player, const ICharacterState<Player>* from, const InputInfo& input) const {
    if (player.IsGrounded()) return PlayerActions::TryStart(player, from, ToInput(input));
    return TryStartAir(player, from, ToInput(input));
}

void PlayerActions::PlayZoomFlash(float zoomPunch) {
    auto* effects = EffectManager::Get();
    if (!effects || zoomPunch <= 0.0f) return;

    effects->ZoomPunch(zoomPunch, ZOOM_FLASH.zoomTime);
    effects->FlashScreen(ZOOM_FLASH.flashColor, zoomPunch * ZOOM_FLASH.flashAlphaPerDegree, ZOOM_FLASH.flashTime);
}
