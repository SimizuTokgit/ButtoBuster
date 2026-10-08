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

    // 回避の残りを 1 回分使う 回避している間は戻らない
    player.UseDodge();

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

    // 反撃できる間は、走り抜けるのを待たずに技へ移れる
    // ジャスト回避が決まったときはふつう相手へ寄るので、ここを通るのは寄る相手がいないとき (針をかわしたとき) と、反撃の途中で回避したとき
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

float PlayerDodgeState::GetOpeningTime(const Player& player) const {
    // 走り抜けて止まり、次の行動を受け付けるまでの残り
    float left = player.data.dodgeTime + player.data.dodgeRecoveryTime - _timer;
    return (left > 0.0f) ? left : 0.0f;
}

void PlayerDodgeState::Exit(Player& player) {
    player.SetOpacity(1.0f);
    player.StopHorizontal();
}
