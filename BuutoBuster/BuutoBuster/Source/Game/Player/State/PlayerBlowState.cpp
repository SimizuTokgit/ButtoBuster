#include "PlayerBlowState.h"
#include "Player.h"
#include "PlayerIdleState.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void PlayerBlowState::Enter(Player& player) {
    player.PlayAnimation("BlowIn", 1.0f, true);
    player.SetKnockback(_knockback);
    player.SetVerticalVelocity(player.params.blownJumpSpeed);

    // 起き上がるまでは何も当たらない 倒れたところを殴られ続けないように
    player.SetInvincible(3.0f);

    // 飛ばされた向きの逆、つまり相手のほうを向いて倒れる
    player.FaceImmediately(VScale(_knockback, -1.0f));
    _phase = Phase::Fly;
}

void PlayerBlowState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    switch (_phase) {
    case Phase::Fly:
        player.DampHorizontal(2.0f, deltaTime);
        if (player.IsAnimationFinished() && player.IsGrounded()) {
            player.StopHorizontal();
            player.PlayAnimation("DownLoop");
            if (auto* effects = EffectManager::Get()) effects->PlayDust(player.GetPosition(), 10);
            _phase = Phase::Down;
            _timer = 0.0f;
        }
        break;

    case Phase::Down:
        _timer += deltaTime;
        if (_timer > player.params.blownDownTime) {
            player.PlayAnimation("BlowOut", 1.3f, true);
            _phase = Phase::GetUp;
        }
        break;

    case Phase::GetUp:
        if (player.IsAnimationFinished()) {
            // 倒れている間の無敵は長めに取ってあるので、起き上がったら少しだけ残す
            player.ResetInvincible(player.params.getUpInvincibleTime);
            player.GetStates().Transition(this, make_unique<PlayerIdleState>());
        }
        break;
    }
}
