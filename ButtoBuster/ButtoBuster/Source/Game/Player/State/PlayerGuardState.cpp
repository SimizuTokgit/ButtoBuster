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
    player.DampHorizontal(player.data.guardDamp, deltaTime);

    // 構えたまま向きだけは変えられる 回り込んでくる敵に向き直るため
    float turnSpeed = player.data.turnSpeed * player.data.holdTurnRate;
    player.FaceTowards(input.move, turnSpeed, deltaTime);

    if (player.ConsumeGuardImpact() && _phase != Phase::Out) {
        player.PlayAnimation("GuardImpact", 1.0f, true);
        _phase = Phase::Impact;
    }

    // 構えたまま押した技を出す ガードを握ったまま押した攻撃とジャンプは、PlayerController が強攻撃と回避にしてある
    // 握っていることは渡さない 出せない技だったときに、ガードへ入り直さないように
    InputInfo fromGuard = input;
    fromGuard.isGuardHeld = false;
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
