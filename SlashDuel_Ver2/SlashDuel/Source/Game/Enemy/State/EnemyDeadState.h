#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Enemy;

class EnemyDeadState : public ICharacterState<Enemy> {
private:
    enum class Phase {
        Fall,
        Lie,
        Sink,
    };

    VECTOR _knockback;
    Phase _phase = Phase::Fall;
    float _timer = 0.0f;

public:
    explicit EnemyDeadState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Dead"; }
};
