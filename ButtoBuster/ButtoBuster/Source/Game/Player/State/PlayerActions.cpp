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

    // 必殺技はゲージが満タンのときだけ 地上でだけ出せる
    case Technique::Special:
        if (!player.IsSpecialReady()) return false;
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
