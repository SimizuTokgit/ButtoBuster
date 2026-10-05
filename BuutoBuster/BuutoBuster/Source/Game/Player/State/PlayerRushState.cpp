#include "PlayerRushState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerIdleState.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

namespace {
    // 止まる所までこれより近ければ着いたとみなす
    constexpr float ARRIVE_DISTANCE = 5.0f;

    // 寄る間は走りのアニメを速く流す 寄るための専用のアニメが無いので
    constexpr float RUN_ANIMATION_SPEED = 2.0f;
}

void PlayerRushState::Enter(Player& player) {
    VECTOR fromTarget = VSub(player.GetPosition(), _targetPosition);
    fromTarget.y = 0.0f;
    float distance = VSize(fromTarget);
    VECTOR outward = (distance > 0.001f) ? VScale(fromTarget, 1.0f / distance) : VScale(player.GetForward(), -1.0f);

    // 体どうしが重ならないよう、両方の太さと少しの隙間を空けた敵の手前で止まる
    float stopDistance = _targetRadius + player.bodyRadius + player.data.counterRushGap;
    _destination = VAdd(_targetPosition, VScale(outward, stopDistance));

    player.FaceImmediately(VScale(outward, -1.0f));
    player.PlayAnimation("Run", RUN_ANIMATION_SPEED, true);

    // 踏み切った土煙
    auto* effects = EffectManager::Get();
    if (effects && player.IsGrounded()) effects->PlayDust(player.GetPosition(), 6);
}

void PlayerRushState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    _timer += deltaTime;
    if (input.technique != Technique::None) _queued = input.technique;

    VECTOR toDestination = VSub(_destination, player.GetPosition());
    toDestination.y = 0.0f;
    float distance = VSize(toDestination);

    bool hasArrived = distance <= ARRIVE_DISTANCE || _timer >= player.data.counterRushMaxTime;
    if (!hasArrived) {
        // 行き過ぎないよう、このフレームで詰めきれる距離なら速さを落とす
        float speed = player.data.counterRushSpeed;
        if (deltaTime > 0.0f && distance / deltaTime < speed) speed = distance / deltaTime;
        player.SetHorizontalVelocity(VScale(toDestination, 1.0f / distance), speed);
        return;
    }

    player.StopHorizontal();

    // 寄っている間に押してあった技を出す 押していなければ構えて待つ
    InputInfo queuedInput = input;
    queuedInput.technique = _queued;
    if (PlayerActions::TryStart(player, this, queuedInput)) return;
    player.GetStates().Transition(this, make_unique<PlayerIdleState>());
}

void PlayerRushState::Exit(Player& player) {
    player.StopHorizontal();
}
