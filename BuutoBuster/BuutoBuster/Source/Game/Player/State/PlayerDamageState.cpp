#include "PlayerDamageState.h"
#include "Player.h"
#include "PlayerIdleState.h"
#include <memory>

using std::make_unique;

void PlayerDamageState::Enter(Player& player) {
    player.PlayAnimation("Damage", 1.3f, true);
    player.SetKnockback(_knockback);
    player.SetInvincible(player.params.hurtInvincibleTime);
}

void PlayerDamageState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    player.DampHorizontal(8.0f, deltaTime);

    // アニメの最後まで待つと重いので、崩れた姿勢が戻ったところで動けるようにする
    if (player.GetAnimationTime() > 14.0f || player.IsAnimationFinished()) {
        player.GetStates().Transition(this, make_unique<PlayerIdleState>());
    }
}
