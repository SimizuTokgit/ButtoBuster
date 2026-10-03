#pragma once
#include "ICharacterState.h"

class Enemy;

class EnemyIdleState : public ICharacterState<Enemy> {
public:
    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Idle"; }
};
