#pragma once
#include "ICharacterState.h"

class Enemy;

class EnemyMoveState : public ICharacterState<Enemy> {
public:
    // 移動の倒し具合がこれ以下なら止まる 待機 (EnemyIdleState) から歩き出すのも、これを越えたとき
    static constexpr float DEADZONE = 0.05f;

private:
    // これより大きく倒されていたら走る 小さければ歩く
    static constexpr float RUN_THRESHOLD = 0.6f;

    bool _isRunning = false;

public:
    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return _isRunning ? "Run" : "Walk"; }
};
