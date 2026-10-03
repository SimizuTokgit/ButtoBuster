#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Player;

class PlayerDeadState : public ICharacterState<Player> {
private:
    VECTOR _knockback;
    bool _isDown = false;

public:
    explicit PlayerDeadState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Dead"; }
};
