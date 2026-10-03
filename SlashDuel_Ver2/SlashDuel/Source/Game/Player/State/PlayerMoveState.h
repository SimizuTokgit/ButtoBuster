#pragma once
#include "ICharacterState.h"

class Player;

class PlayerMoveState : public ICharacterState<Player> {
public:
    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Move"; }
};
