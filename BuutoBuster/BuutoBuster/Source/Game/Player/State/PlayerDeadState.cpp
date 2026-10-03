#include "PlayerDeadState.h"
#include "Player.h"
#include "EffectManager.h"

void PlayerDeadState::Enter(Player& player) {
    player.PlayAnimation("BlowIn", 1.0f, true);
    player.SetKnockback(_knockback);
    player.SetVerticalVelocity(420.0f);
    player.SetTrailEmitting(false);
}

void PlayerDeadState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    if (_isDown) {
        player.StopHorizontal();
        return;
    }

    player.DampHorizontal(2.0f, deltaTime);
    if (player.IsAnimationFinished() && player.IsGrounded()) {
        player.PlayAnimation("DownLoop");
        if (auto* effects = EffectManager::Get()) effects->PlayDust(player.GetPosition(), 12);
        _isDown = true;
    }
}
