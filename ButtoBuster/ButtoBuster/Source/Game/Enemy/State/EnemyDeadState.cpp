#include "EnemyDeadState.h"
#include "Enemy.h"

void EnemyDeadState::Enter(Enemy& enemy) {
    const EnemyData& data = enemy.GetData();

    // 倒された体で生きている相手を押さない
    enemy.SetBodySolid(false);
    enemy.SetHovering(false);
    enemy.SetKnockback(_knockback);
    enemy.SetVerticalVelocity(JUMP_SPEED);

    if (data.canBlow && !data.isFlying) {
        // 飛ばされた向きの逆を向いたまま、後ろ向きに飛んでいく
        enemy.PlayAnimation("BlowIn", 1.0f, true);
        enemy.FaceImmediately(VScale(_knockback, -1.0f));
    }
    else {
        enemy.PlayAnimation("Down", 1.0f, true);
    }

    _phase = Phase::Fly;
    _timer = 0.0f;
}

void EnemyDeadState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    _timer += deltaTime;

    switch (_phase) {
    case Phase::Fly:
        if (_timer > FLY_TIME) {
            _phase = Phase::Fade;
            _timer = 0.0f;
        }
        break;

    case Phase::Fade: {
        float rate = _timer / FADE_TIME;
        if (rate >= 1.0f) {
            rate = 1.0f;
            enemy.MarkReadyToRemove();
        }
        enemy.SetOpacity(1.0f - rate);
        break;
    }
    }
}
