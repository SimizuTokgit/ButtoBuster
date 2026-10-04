#include "PlayerDeadState.h"
#include "Player.h"
#include "EffectManager.h"

void PlayerDeadState::Enter(Player& player) {
    player.PlayAnimation("BlowIn", 1.0f, true);
    player.SetKnockback(_knockback);
    player.SetVerticalVelocity(JUMP_SPEED);
    player.SetTrailEmitting(false);

    // 飛ばされた向きの逆を向いたまま、後ろ向きに飛んでいく
    player.FaceImmediately(VScale(_knockback, -1.0f));
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
