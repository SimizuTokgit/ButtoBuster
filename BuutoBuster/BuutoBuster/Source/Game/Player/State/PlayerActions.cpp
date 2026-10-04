#include "PlayerActions.h"
#include "Player.h"
#include "PlayerAttacks.h"
#include "PlayerAttackState.h"
#include "PlayerChargeState.h"
#include "PlayerDodgeState.h"
#include "PlayerGuardState.h"
#include "PlayerJumpState.h"
#include <memory>

using std::make_unique;

namespace {
    // スティックを倒したとみなす量
    constexpr float MOVE_THRESHOLD = 0.1f;
}

bool PlayerActions::HasMoveInput(const InputInfo& input) {
    return VSize(input.move) > MOVE_THRESHOLD;
}

bool PlayerActions::TryStart(Player& player, const ICharacterState<Player>* from, const InputInfo& input) {
    auto& states = player.GetStates();

    switch (input.technique) {
    case Technique::Slash:
        return states.Transition(from, make_unique<PlayerAttackState>(PlayerAttacks::GetSlash(0), 0, false));

    // 強攻撃はヘビーアタック 押し続けると溜まり、すぐ離せば溜めずに振る
    case Technique::StrongSlash:
        return states.Transition(from, make_unique<PlayerChargeState>());

    case Technique::AntiAir:
        return states.Transition(from, make_unique<PlayerAttackState>(PlayerAttacks::GetAntiAir(), -1, true));

    case Technique::Dodge: {
        // 倒していなければ後ろへ下がる
        VECTOR direction = HasMoveInput(input) ? input.move : VScale(player.GetForward(), -1.0f);
        return states.Transition(from, make_unique<PlayerDodgeState>(direction));
    }

    case Technique::Jump:
        if (!player.IsGrounded()) return false;
        return states.Transition(from, make_unique<PlayerJumpState>(true));

    default:
        break;
    }

    if (input.isGuardHeld) {
        return states.Transition(from, make_unique<PlayerGuardState>());
    }

    return false;
}
