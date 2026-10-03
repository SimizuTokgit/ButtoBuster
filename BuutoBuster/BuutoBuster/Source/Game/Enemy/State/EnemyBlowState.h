#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Enemy;

// 吹き飛んで倒れ、起き上がるまで
// 空を飛ぶ敵は地面まで落ちて、しばらくしてから飛び直す
class EnemyBlowState : public ICharacterState<Enemy> {
private:
    static constexpr float DOWN_TIME = 0.6f;

    // 落とされた Bee が地面にいる時間 ここで地上の斬りが届く
    static constexpr float GROUNDED_TIME = 1.4f;

    enum class Phase {
        Fly,
        Down,
        GetUp,
    };

    VECTOR _knockback;
    Phase _phase = Phase::Fly;
    float _timer = 0.0f;

public:
    explicit EnemyBlowState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    void Exit(Enemy& enemy) override;
    const char* GetName() const override { return "Blow"; }
};
