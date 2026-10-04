#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Enemy;

class EnemyDamageState : public ICharacterState<Enemy> {
private:
    VECTOR _knockback;

public:
    explicit EnemyDamageState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Damage"; }
};
