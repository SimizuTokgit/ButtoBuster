#include "PlayerBlowState.h"
#include "Player.h"
#include "PlayerIdleState.h"
#include "ArenaWall.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

void PlayerBlowState::Enter(Player& player) {
    player.PlayAnimation("BlowIn", 1.0f, true);
    player.SetKnockback(_knockback);

    // 吹っ飛ぶ角度 (Character::LAUNCH_ANGLE) で斜め上へ飛ぶ 弱く飛ばされても blownJumpSpeed だけは跳ねる
    player.SetVerticalVelocity(Character::GetLaunchUpSpeed(_knockback, player.data.blownJumpSpeed));
    player.ForgetWallHit();

    // 起き上がるまでは何も当たらない 倒れたところを殴られ続けないように
    player.SetInvincible(3.0f);

    // 飛ばされた向きの逆、つまり相手のほうを向いて倒れる
    player.FaceImmediately(VScale(_knockback, -1.0f));
    _phase = Phase::Fly;
}

void PlayerBlowState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    switch (_phase) {
    case Phase::Fly: {
        // 飛んでいる間は吹っ飛ばされ値を減らさない
        player.HoldBlow();

        // 壁にぶつかったら、吹っ飛ばされ値が許容値に届いていれば壁を割られて負け、届いていなければ張り付いてから跳ね返る
        ArenaWall::Hit wallHit;
        if (player.ConsumeWallHit(wallHit)) {
            ArenaWall::Reaction reaction = ArenaWall::React(player, wallHit);

            // 割られたときは場外へ飛んでいく状態に移るので、ここから先はいらない
            if (reaction == ArenaWall::Reaction::Break) break;

            if (reaction == ArenaWall::Reaction::Bounce) {
                // ぶつかった姿のまま壁に止める
                _wallHit = wallHit;
                _stickTime = ArenaWall::GetStickTime(wallHit);
                _timer = 0.0f;
                player.StopHorizontal();
                player.SetVerticalVelocity(0.0f);
                player.SetAnimationSpeed(0.0f);
                _phase = Phase::Stick;
                break;
            }
        }

        player.DampHorizontal(2.0f, deltaTime);
        if (player.IsAnimationFinished() && player.IsGrounded()) {
            player.StopHorizontal();
            player.PlayAnimation("DownLoop");
            if (auto* effects = EffectManager::Get()) effects->PlayDust(player.GetPosition(), 10);
            _phase = Phase::Down;
            _timer = 0.0f;
        }
        break;
    }

    case Phase::Stick:
        player.HoldBlow();

        // 張り付いている間は壁に止めておく
        player.StopHorizontal();
        player.SetVerticalVelocity(0.0f);

        _timer += deltaTime;
        if (_timer < _stickTime) break;

        // 飛ばされた向きの逆を向いて飛ぶのは、跳ね返ったあとも同じ
        ArenaWall::Bounce(player, _wallHit);
        player.PlayAnimation("BlowIn", 1.0f, true);
        player.FaceImmediately(VScale(player.GetVelocity(), -1.0f));
        _phase = Phase::Fly;
        break;

    case Phase::Down:
        _timer += deltaTime;
        if (_timer > player.data.blownDownTime) {
            // 起き上がるときに吹っ飛ばされ値を少し戻す
            player.RecoverBlowOnGetUp();
            player.PlayAnimation("BlowOut", 1.3f, true);
            _phase = Phase::GetUp;
        }
        break;

    case Phase::GetUp:
        if (player.IsAnimationFinished()) {
            // 倒れている間の無敵は長めに取ってあるので、起き上がったら少しだけ残す
            player.ResetInvincible(player.data.getUpInvincibleTime);
            player.GetStates().Transition(this, make_unique<PlayerIdleState>());
        }
        break;
    }
}
