#include "PlayerSpecialState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerAttacks.h"
#include "PlayerIdleState.h"
#include "CombatSystem.h"
#include <memory>
#include <vector>

using std::make_unique;

void PlayerSpecialState::Enter(Player& player) {
    // ゲージを空にし、撃ってすぐの間は溜まらないようにする
    player.UseSpecial();

    // 雷を呼んでいる間と、振り抜いて動けるようになるまでは攻撃を受けない
    player.SetInvincible(player.data.specialCallTime + player.data.specialRecoveryTime);

    player.SetTrailEmitting(false);
    player.StopHorizontal();
    HoldPose(player);

    SpecialEvent event;
    event.type = SpecialEvent::Type::Call;
    event.position = player.GetPosition();
    event.callTime = player.data.specialCallTime;
    player.GetSpecialEvents().Notify(event);
}

void PlayerSpecialState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    _timer += deltaTime;

    // その場で踏ん張って動かない
    player.StopHorizontal();

    switch (_phase) {
    case Phase::Call:
        HoldPose(player);
        if (_timer >= player.data.specialCallTime) Strike(player);
        break;

    case Phase::Recover: {
        player.SetTrailEmitting(player.GetAnimationTime() <= SWING_END_TIME);

        if (input.technique != Technique::None) _queued = input.technique;
        if (_timer < player.data.specialRecoveryTime) break;

        InputInfo buffered = input;
        if (_queued != Technique::None) buffered.technique = _queued;
        if (PlayerActions::TryStart(player, this, buffered)) return;

        player.GetStates().Transition(this, make_unique<PlayerIdleState>());
        break;
    }
    }
}

void PlayerSpecialState::Exit(Player& player) {
    // 止めたアニメを次の状態に持ち越さない
    player.SetAnimationSpeed(1.0f);
    player.SetTrailEmitting(false);
}

void PlayerSpecialState::Strike(Player& player) {
    _phase = Phase::Recover;
    _timer = 0.0f;

    // 掲げた剣を振り抜く 刃の軌跡も出す
    player.SetAnimationSpeed(1.0f);
    player.SetTrailEmitting(true);

    // 場にいる敵全員に当てる 強化の倍率は普段の技と同じように掛ける
    // 連鎖は渡さない 雷で飛んだ敵の連鎖で、ゲージがまた溜まらないように コンボの数にも入れない
    AttackData attack = PlayerAttacks::ApplyRates(PlayerAttacks::GetSpecial(), player.data);
    std::vector<Character*> hitList;
    CombatSystem::ApplyAll(player, attack, hitList);

    SpecialEvent event;
    event.type = SpecialEvent::Type::Strike;
    event.position = player.GetPosition();
    for (const Character* target : hitList) event.targets.push_back(target->GetPosition());
    player.GetSpecialEvents().Notify(event);
}

void PlayerSpecialState::HoldPose(Player& player) const {
    // 溜めと同じ姿勢で剣を掲げたまま止める 姿勢は PlayerData の chargePose で始まる値
    player.PlayAnimation(player.data.chargePoseAnimation, 0.0f);
    player.SetAnimationTime(player.data.chargePoseTime);
}
