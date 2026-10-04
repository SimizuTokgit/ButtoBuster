#include "PlayerMoveState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerIdleState.h"
#include <memory>

using std::make_unique;

void PlayerMoveState::Enter(Player& player) {
    player.PlayAnimation("Run");
}

void PlayerMoveState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    if (PlayerActions::TryStart(player, this, input)) return;

    if (!PlayerActions::HasMoveInput(input)) {
        player.GetStates().Transition(this, make_unique<PlayerIdleState>());
        return;
    }

    player.SetHorizontalVelocity(input.move, player.data.moveSpeed);
    player.FaceTowards(input.move, player.data.turnSpeed, deltaTime);

    // 少しだけ倒したときは足の運びもゆっくりにする
    float amount = VSize(input.move);
    if (amount > 1.0f) amount = 1.0f;
    player.SetAnimationSpeed(0.5f + amount * 0.5f);
}
