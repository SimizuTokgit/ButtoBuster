#include "PlayerChargeState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerAttacks.h"
#include "PlayerAttackState.h"
#include <memory>

using std::make_unique;

void PlayerChargeState::Enter(Player& player) {
    player.SetTrailEmitting(false);
    player.StopHorizontal();
    player.HoldChargePose();

    Notify(player, PlayerChargeEvent::Type::Start);
}

void PlayerChargeState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    // 溜めている途中でも回避だけはできる 囲まれたときに溜めを捨てて逃げられるように
    if (input.technique == Technique::Dodge && PlayerActions::TryStart(player, this, input)) return;

    // 毎フレーム姿勢をかけ直す デバッグで姿勢を書き換えたとき、溜めたままその場で変わるように
    player.HoldChargePose();

    // その場で踏ん張って動かない 走りながら溜め始めても滑らないよう毎フレーム止める
    // 向きだけは変えて狙える
    player.StopHorizontal();
    const PlayerData& data = player.data;
    player.FaceTowards(input.move, data.turnSpeed * data.holdTurnRate, deltaTime);

    if (!input.isHeavyHeld) {
        Release(player);
        return;
    }

    _chargeTime += deltaTime;
    if (_level < PlayerAttacks::CHARGE_LEVEL_MAX && _chargeTime >= data.chargeLevelUpTimes[_level]) {
        _level++;
        Notify(player, PlayerChargeEvent::Type::LevelUp);
    }
}

void PlayerChargeState::Exit(Player& player) {
    // 止めたアニメを次の状態に持ち越さない 次の状態がアニメを替えなくても動くように
    player.SetAnimationSpeed(1.0f);

    if (!_hasReleased) Notify(player, PlayerChargeEvent::Type::Cancel);
}

const char* PlayerChargeState::GetName() const {
    static const char* names[] = { "Charge0", "Charge1", "Charge2", "Charge3" };
    return names[_level];
}

void PlayerChargeState::Release(Player& player) {
    const AttackData& attack = PlayerAttacks::GetHeavy(_level);
    bool isStarted = player.GetStates().Transition(this,
        make_unique<PlayerAttackState>(attack, -1, PlayerAttackState::Kind::Ground, _level));
    if (!isStarted) return;

    _hasReleased = true;
    Notify(player, PlayerChargeEvent::Type::Release);
}

void PlayerChargeState::Notify(Player& player, PlayerChargeEvent::Type type) const {
    PlayerChargeEvent event;
    event.type = type;
    event.level = _level;
    event.maxLevel = PlayerAttacks::CHARGE_LEVEL_MAX;
    event.position = player.GetPosition();
    event.headPosition = player.GetHeadPosition();
    player.GetChargeEvents().Notify(event);
}
