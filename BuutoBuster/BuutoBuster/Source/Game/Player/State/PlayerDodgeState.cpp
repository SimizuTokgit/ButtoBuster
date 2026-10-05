#include "PlayerDodgeState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerIdleState.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void PlayerDodgeState::Enter(Player& player) {
    _direction.y = 0.0f;
    float length = VSize(_direction);
    _direction = (length > 0.001f) ? VScale(_direction, 1.0f / length) : VScale(player.GetForward(), -1.0f);

    // 前や横へ抜けるときはそちらを向く 後ろへ下がるときは相手を見たまま
    if (VDot(_direction, player.GetForward()) > -0.5f) {
        player.FaceImmediately(_direction);
    }

    player.SetInvincible(player.data.dodgeInvincibleTime);
    player.SetOpacity(0.45f);

    // 踏み切った直後に攻撃が来たら、ジャスト回避になる
    player.OpenJustDodgeWindow();

    // 地面を蹴った土煙 どちらへ抜けたかが残る
    auto* effects = EffectManager::Get();
    if (effects && player.IsGrounded()) effects->PlayDust(player.GetPosition(), 6);

    // 回避専用のアニメが無いので、踏み切りの動きを速く流して代わりにする
    // 音もこのアニメに付けてあるものが鳴る
    player.PlayAnimation("JumpIn", 1.6f, true);
}

void PlayerDodgeState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    _timer += deltaTime;

    // ジャスト回避が決まったら、走り抜けるのを待たずに反撃へ移れる
    // ふつうは決まった瞬間に相手へ寄るのでここは通らない 針をかわしたときのように、寄る相手がいないときに使う
    bool canCounter = player.HasCounter() && input.technique != Technique::None;
    if (canCounter && PlayerActions::TryStart(player, this, input)) return;

    const PlayerData& data = player.data;
    if (_timer < data.dodgeTime) {
        player.SetKnockback(VScale(_direction, data.dodgeSpeed));
        return;
    }

    player.DampHorizontal(18.0f, deltaTime);
    player.SetOpacity(1.0f);

    if (_timer < data.dodgeTime + data.dodgeRecoveryTime) return;

    if (PlayerActions::TryStart(player, this, input)) return;
    player.GetStates().Transition(this, make_unique<PlayerIdleState>());
}

void PlayerDodgeState::Exit(Player& player) {
    player.SetOpacity(1.0f);
    player.StopHorizontal();
}
