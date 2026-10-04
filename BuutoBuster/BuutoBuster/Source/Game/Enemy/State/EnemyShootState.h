#pragma once
#include "ICharacterState.h"

class Enemy;

// Bee が Needle を撃つ
class EnemyShootState : public ICharacterState<Enemy> {
private:
    bool _hasShot = false;

public:
    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    void Exit(Enemy& enemy) override;
    const char* GetName() const override { return "Shoot"; }
};
