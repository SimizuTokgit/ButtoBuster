#include "PlayerDamageState.h"
#include "Player.h"
#include "PlayerIdleState.h"
#include <memory>

using std::make_unique;

namespace {
    // のけぞりのアニメを流す速さ 1 でふだんどおり
    constexpr float ANIMATION_SPEED = 1.3f;

    // 崩れた姿勢が戻るフレーム
    constexpr float RECOVER_FRAME = 14.0f;
}

void PlayerDamageState::Enter(Player& player) {
    player.PlayAnimation("Damage", ANIMATION_SPEED, true);
    player.SetKnockback(_knockback);
    player.SetInvincible(player.data.hurtInvincibleTime);
}

void PlayerDamageState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    player.DampHorizontal(player.data.hurtDamp, deltaTime);

    // アニメの最後まで待つと重いので、崩れた姿勢が戻ったところで動けるようにする
    if (player.GetAnimationTime() > RECOVER_FRAME || player.IsAnimationFinished()) {
        player.GetStates().Transition(this, make_unique<PlayerIdleState>());
    }
}
