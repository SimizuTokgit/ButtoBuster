#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Enemy;

// 壁を割って場外へ飛んでいき、透けて消える
// 倒された体はもう壁に止められず、誰にも当たらない
class EnemyDeadState : public ICharacterState<Enemy> {
private:
    // 割った勢いで上へ跳ねる速さ 場外へ大きく飛んでいくのが見えるように
    static constexpr float JUMP_SPEED = 700.0f;

    // 飛んでいく時間と、そのあと透けて消えるまでの時間 秒
    static constexpr float FLY_TIME = 1.0f;
    static constexpr float FADE_TIME = 0.5f;

    enum class Phase {
        Fly,
        Fade,
    };

    VECTOR _knockback;
    Phase _phase = Phase::Fly;
    float _timer = 0.0f;

public:
    // knockback は場外へ飛んでいく向きと速さ
    explicit EnemyDeadState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Dead"; }
};
