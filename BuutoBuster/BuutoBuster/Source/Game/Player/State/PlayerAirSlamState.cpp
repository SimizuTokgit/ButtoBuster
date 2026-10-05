#include "PlayerAirSlamState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerAttacks.h"
#include "PlayerIdleState.h"
#include "PlayerJumpState.h"
#include "CombatSystem.h"
#include "BlowChain.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include <memory>
#include <vector>

using std::make_unique;

void PlayerAirSlamState::Enter(Player& player) {
    // 強化で上がった倍率を掛ける ジャスト回避のあとなら、さらにこの叩きつけが反撃になる
    _data = PlayerAttacks::ApplyRates(PlayerAttacks::GetAirSlam(), player.data);
    if (player.ConsumeCounter()) _data = PlayerAttacks::CreateCounter(_data, player.data);

    // 宙で止まって振りかぶる 跳んだ勢いも横の勢いも消して、真下へ落ちられるようにする
    player.SetGravityEnabled(false);
    player.SetVerticalVelocity(0.0f);
    player.StopHorizontal();
    player.SetTrailEmitting(false);
    player.PlayAnimation(_data.animationName, 1.0f, true);
}

void PlayerAirSlamState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    _timer += deltaTime;
    player.StopHorizontal();

    switch (_phase) {
    case Phase::Windup: {
        player.SetVerticalVelocity(0.0f);

        bool isPoseReached = player.GetAnimationTime() >= POSE_TIME || player.IsAnimationFinished();
        if (isPoseReached) StartDive(player);
        break;
    }

    case Phase::Dive:
        player.SetVerticalVelocity(-player.data.airSlamDiveSpeed);
        if (player.IsGrounded() || _timer > DIVE_TIME_LIMIT) Land(player);
        break;

    case Phase::Land: {
        player.SetTrailEmitting(player.GetAnimationTime() <= SWING_END_TIME);

        if (input.technique != Technique::None) _queued = input.technique;
        if (_timer < player.data.airSlamRecoveryTime) break;

        // 地面に着かないまま叩きつけたときは、そのまま着地まで落ちる
        if (!player.IsGrounded()) {
            player.GetStates().Transition(this, make_unique<PlayerJumpState>(false));
            return;
        }

        InputInfo buffered = input;
        if (_queued != Technique::None) buffered.technique = _queued;
        if (PlayerActions::TryStart(player, this, buffered)) return;

        if (player.IsAnimationFinished() || PlayerActions::HasMoveInput(input)) {
            player.GetStates().Transition(this, make_unique<PlayerIdleState>());
        }
        break;
    }
    }
}

float PlayerAirSlamState::GetOpeningTime(const Player& player) const {
    // 着地してから動けるまでの残り 宙にいる間は地上の敵の振りが届かないので数えない
    if (_phase != Phase::Land) return 0.0f;

    float left = player.data.airSlamRecoveryTime - _timer;
    return (left > 0.0f) ? left : 0.0f;
}

void PlayerAirSlamState::Exit(Player& player) {
    // 被弾して途中で抜けても、重力と止めたアニメを元に戻す
    player.SetGravityEnabled(true);
    player.SetAnimationSpeed(1.0f);
    player.SetTrailEmitting(false);
}

void PlayerAirSlamState::StartDive(Player& player) {
    _phase = Phase::Dive;
    _timer = 0.0f;

    // 振りかぶった姿勢のまま落ちる 刃の軌跡で、落ちていく線を見せる
    player.SetAnimationSpeed(0.0f);
    player.SetAnimationTime(POSE_TIME);
    player.SetVerticalVelocity(-player.data.airSlamDiveSpeed);
    player.SetTrailEmitting(true);
}

void PlayerAirSlamState::Land(Player& player) {
    _phase = Phase::Land;
    _timer = 0.0f;

    player.SetGravityEnabled(true);
    player.SetVerticalVelocity(0.0f);

    // 止めていた振りかぶりから振り抜く
    player.SetAnimationSpeed(1.0f);

    // 着地した場所から衝撃波で周りの敵を吹き飛ばす 飛ばした敵は連鎖の砲弾になる
    std::vector<Character*> hitList;
    auto chain = BlowChain::Create(&player.GetChainEvents());
    int hits = CombatSystem::ApplyArea(player, _data.shockwaveRadius, _data, hitList, chain);
    if (hits > 0) player.AddCombo(hits);

    SoundManager::Instance().PlaySE("Golem/attack_stomp");

    auto* effects = EffectManager::Get();
    if (!effects) return;

    // 外しても地面は揺らす 叩きつけた重さが伝わるように
    VECTOR position = player.GetPosition();
    effects->PlayShockwave(position, _data.shockwaveRadius, _data.arcColor);
    effects->PlayDust(position, 16);
    effects->Shake(_data.shake, 0.3f);
    if (hits <= 0) return;

    effects->HitStop(_data.hitStop);
    effects->ZoomPunch(_data.zoomPunch, 0.35f);
    effects->FlashScreen(0xFFFFFF, _data.zoomPunch * 0.04f, 0.12f);
}
