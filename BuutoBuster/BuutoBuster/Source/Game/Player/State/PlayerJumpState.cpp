#include "PlayerJumpState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerIdleState.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void PlayerJumpState::Enter(Player& player) {
    if (_hasImpulse) {
        player.SetVerticalVelocity(player.data.jumpSpeed);
        player.PlayAnimation("JumpIn", 1.0f, true);
        _isRising = true;
    }
    else {
        player.PlayAnimation("JumpLoop");
        _isRising = false;
    }
}

void PlayerJumpState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    _airTime += deltaTime;

    if (_isLanding) {
        player.StopHorizontal();
        if (PlayerActions::TryStart(player, this, input)) return;

        bool canMoveOn = player.IsAnimationFinished()
            || (player.GetAnimationTime() > 6.0f && PlayerActions::HasMoveInput(input));
        if (canMoveOn) {
            player.GetStates().Transition(this, make_unique<PlayerIdleState>());
        }
        return;
    }

    // 空中でも少しは曲がれる
    const PlayerData& data = player.data;
    player.SetHorizontalVelocity(input.move, data.moveSpeed * data.airMoveRate);
    player.FaceTowards(input.move, data.turnSpeed, deltaTime);

    if (_isRising && player.IsAnimationFinished()) {
        player.PlayAnimation("JumpLoop");
        _isRising = false;
    }

    // 空中で攻撃を押したら空中の技 攻撃は空中の斬り、攻撃 + ガードは真下への叩きつけ
    if (PlayerActions::TryStartAir(player, this, input)) return;

    if (_airTime > MIN_AIR_TIME && player.IsGrounded()) {
        _isLanding = true;
        player.StopHorizontal();
        player.PlayAnimation("JumpOut", 1.4f, true);
        if (auto* effects = EffectManager::Get()) effects->PlayDust(player.GetPosition(), 8);
    }
}
