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
    // 強化で上がった倍率と OP 相殺を掛ける ジャスト回避のあとなら、さらにこの叩きつけが反撃になる
    // 着地の隙は AttackData の外 (PlayerData) にあるので、OP 相殺で伸ばす分はここで掛ける
    _staleCount = player.UseMove(GetName());
    _data = PlayerAttacks::ApplyRates(PlayerAttacks::GetAirSlam(), player.data, _staleCount);
    _recoveryTime = player.data.airSlamRecoveryTime * PlayerAttacks::GetStaleRecoveryRate(_staleCount, player.data);
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

        bool isPoseReached = player.GetAnimationTime() >= player.data.chargePoseTime || player.IsAnimationFinished();
        if (isPoseReached) StartDive(player);
        break;
    }

    case Phase::Dive:
        player.SetVerticalVelocity(-player.data.airSlamDiveSpeed);
        Catch(player);
        if (player.IsGrounded() || _timer > DIVE_TIME_LIMIT) {
            // 前の扇形で当てるので、振り抜く前に近くの敵へ向きを吸い付ける 地上の斬りと同じ
            player.FaceImmediately(player.FindAimDirection(input.move, player.data.aimRadius));
            Land(player);
        }
        break;

    case Phase::Land: {
        player.SetTrailEmitting(player.GetAnimationTime() <= player.data.chargeSwingEndTime);

        _queued.Remember(input);
        if (_timer < _recoveryTime) break;

        // 地面に着かないまま叩きつけたときは、そのまま着地まで落ちる
        if (!player.IsGrounded()) {
            player.GetStates().Transition(this, make_unique<PlayerJumpState>(false));
            return;
        }

        if (_queued.TryStart(player, this, input)) return;

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

    float left = _recoveryTime - _timer;
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
    player.SetAnimationTime(player.data.chargePoseTime);
    player.SetVerticalVelocity(-player.data.airSlamDiveSpeed);
    player.SetTrailEmitting(true);
}

void PlayerAirSlamState::Catch(Player& player) {
    // 宙の敵をのけぞらせ、自分と同じ速さで真下へ落とす 地面に着いたら敵の方で跳ね上がる
    AttackData attack = PlayerAttacks::ApplyRates(PlayerAttacks::GetAirSlamCatch(), player.data, _staleCount);
    attack.lift = -player.data.airSlamDiveSpeed;

    int hits = CombatSystem::ApplyArea(player, PlayerAttacks::AIR_SLAM_CATCH_RADIUS, attack, _caught);
    if (hits > 0) player.AddCombo(hits);
}

void PlayerAirSlamState::Land(Player& player) {
    _phase = Phase::Land;
    _timer = 0.0f;

    player.SetGravityEnabled(true);
    player.SetVerticalVelocity(0.0f);

    // 止めていた振りかぶりから振り抜く
    player.SetAnimationSpeed(1.0f);

    // 前の扇形の敵を吹き飛ばす 飛ばした敵は連鎖の砲弾になる
    // 落ちる途中で捕まえた敵は当てた扱いにして外し、地面で跳ね上がったまま残す
    auto chain = BlowChain::Create(&player.GetChainEvents());
    int hits = CombatSystem::ApplyMelee(player, _data, _caught, chain);
    if (hits > 0) player.AddCombo(hits);

    // 地面を叩く音と、締めの斬りと同じ声
    SoundManager::Instance().PlaySE("Golem/attack_stomp");
    SoundManager::Instance().PlaySE(PlayerAttacks::SLASH_VOICES[PlayerAttacks::SLASH_COUNT - 1], PlayerData::VOICE_VOLUME);

    auto* effects = EffectManager::Get();
    if (!effects) return;

    // 外しても地面は揺らす 叩きつけた重さが伝わるように
    VECTOR position = player.GetPosition();
    effects->PlayShockwave(position, _data.shockwaveRadius, _data.arcColor);
    effects->PlayDust(position, player.data.airSlamDustCount);
    effects->Shake(_data.shake, LAND_SHAKE_TIME);
    if (hits <= 0) return;

    effects->HitStop(_data.hitStop);
    PlayerActions::PlayZoomFlash(_data.zoomPunch);
}
