#include "PlayerIdleState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerMoveState.h"
#include <memory>

using std::make_unique;

void PlayerIdleState::Enter(Player& player) {
    player.PlayAnimation("Neutral");
    player.StopHorizontal();
}

void PlayerIdleState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    if (PlayerActions::TryStart(player, this, input)) return;

    if (PlayerActions::HasMoveInput(input)) {
        player.GetStates().Transition(this, make_unique<PlayerMoveState>());
        return;
    }

    player.StopHorizontal();
}
