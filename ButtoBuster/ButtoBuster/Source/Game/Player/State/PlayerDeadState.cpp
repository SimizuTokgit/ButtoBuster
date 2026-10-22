#include "PlayerDeadState.h"
#include "Player.h"
#include "PlayerBlowState.h"

void PlayerDeadState::Enter(Player& player) {
    // 飛ばされた向きの逆を向いたまま、後ろ向きに飛んでいく
    player.PlayBlowIn(_knockback);
    player.SetKnockback(_knockback);
    player.SetVerticalVelocity(Character::DEFEAT_JUMP_SPEED);
    player.SetTrailEmitting(false);
}

void PlayerDeadState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    if (_isDown) {
        player.StopHorizontal();
        return;
    }

    // 落ち方は吹き飛んだときと同じ
    _isDown = PlayerBlowState::FallDown(player, DUST_COUNT, deltaTime);
}
