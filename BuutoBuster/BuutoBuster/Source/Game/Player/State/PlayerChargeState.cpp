#include "PlayerChargeState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerAttacks.h"
#include "PlayerAttackState.h"
#include <memory>

using std::make_unique;

namespace {
    // 押し始めてからこの秒数で 1 2 3 段目に上がる
    // 1 段目より前に離したら、溜めていない普通のヘビーアタックになる
    constexpr float LEVEL_UP_TIMES[] = { 0.35f, 0.85f, 1.4f };
    static_assert(sizeof(LEVEL_UP_TIMES) / sizeof(LEVEL_UP_TIMES[0]) == PlayerAttacks::CHARGE_LEVEL_MAX,
        "段階の数と上がる時間の数をそろえる");

    // 頭の上のどこに溜めの光を出すか
    constexpr float HEAD_OFFSET = 30.0f;
}

void PlayerChargeState::Enter(Player& player) {
    player.SetTrailEmitting(false);
    player.StopHorizontal();
    HoldPose(player);

    Notify(player, PlayerChargeEvent::Type::Start);
}

void PlayerChargeState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    // 溜めている途中でも回避だけはできる 囲まれたときに溜めを捨てて逃げられるように
    if (input.technique == Technique::Dodge && PlayerActions::TryStart(player, this, input)) return;

    // 毎フレーム姿勢をかけ直す デバッグで姿勢を書き換えたとき、溜めたままその場で変わるように
    HoldPose(player);

    // その場で踏ん張って動かない 走りながら溜め始めても滑らないよう毎フレーム止める
    // 向きだけは変えて狙える ロックオン中は相手を向き続ける
    player.StopHorizontal();
    bool isLockedOn = VSquareSize(input.look) > 0.0001f;
    player.FaceTowards(isLockedOn ? input.look : input.move, Player::TURN_SPEED * 0.5f, deltaTime);

    if (!input.isHeavyHeld) {
        Release(player);
        return;
    }

    _chargeTime += deltaTime;
    if (_level < PlayerAttacks::CHARGE_LEVEL_MAX && _chargeTime >= LEVEL_UP_TIMES[_level]) {
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

void PlayerChargeState::HoldPose(Player& player) const {
    // 速さ 0 で止めたまま、決めた時間の姿勢にする 間を再生しないので、アニメに付けた音は鳴らない
    player.PlayAnimation(player.chargePoseAnimation, 0.0f);
    player.SetAnimationTime(player.chargePoseTime);
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
    event.headPosition = VAdd(event.position, VGet(0.0f, player.bodyHeight + HEAD_OFFSET, 0.0f));
    player.GetChargeEvents().Notify(event);
}
