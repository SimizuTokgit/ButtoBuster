#include "PlayerGuardState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerIdleState.h"
#include <memory>

using std::make_unique;

void PlayerGuardState::Enter(Player& player) {
    player.SetGuarding(true);
    player.StopHorizontal();
    player.PlayAnimation("GuardIn", 1.0f, true);
    _phase = Phase::In;
}

void PlayerGuardState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    // 押された分だけ滑って止まる
    player.DampHorizontal(10.0f, deltaTime);

    // 構えたまま向きだけは変えられる 回り込んでくる敵に向き直るため
    // ロックオン中は相手へ盾を向け続ける
    bool isLockedOn = VSquareSize(input.look) > 0.0001f;
    float turnSpeed = player.params.turnSpeed * player.params.holdTurnRate;
    player.FaceTowards(isLockedOn ? input.look : input.move, turnSpeed, deltaTime);

    if (player.ConsumeGuardImpact() && _phase != Phase::Out) {
        player.PlayAnimation("GuardImpact", 1.0f, true);
        _phase = Phase::Impact;
    }

    // 構えたまま押した攻撃とジャンプは、同時押しと同じ技にする
    // ガードを先に押してから少し遅れて押しても、組み合わせの技が出るように
    InputInfo fromGuard = input;
    fromGuard.isGuardHeld = false;
    if (input.isGuardHeld) {
        if (fromGuard.technique == Technique::Slash) fromGuard.technique = Technique::StrongSlash;
        if (fromGuard.technique == Technique::Jump) fromGuard.technique = Technique::Dodge;
    }
    if (fromGuard.technique != Technique::None
        && PlayerActions::TryStart(player, this, fromGuard)) {
        return;
    }

    if (_phase != Phase::Out && !input.isGuardHeld) {
        _phase = Phase::Out;
        player.SetGuarding(false);
        player.PlayAnimation("GuardOut", 1.0f, true);
        return;
    }

    switch (_phase) {
    case Phase::In:
    case Phase::Impact:
        if (player.IsAnimationFinished()) {
            player.PlayAnimation("GuardLoop");
            _phase = Phase::Hold;
        }
        break;

    case Phase::Out:
        if (player.IsAnimationFinished() || PlayerActions::HasMoveInput(input)) {
            player.GetStates().Transition(this, make_unique<PlayerIdleState>());
        }
        break;

    default:
        break;
    }
}

void PlayerGuardState::Exit(Player& player) {
    player.SetGuarding(false);
}
