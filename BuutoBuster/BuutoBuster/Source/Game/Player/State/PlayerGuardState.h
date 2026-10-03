#pragma once
#include "ICharacterState.h"

class Player;

class PlayerGuardState : public ICharacterState<Player> {
private:
    enum class Phase {
        In,
        Hold,
        Impact,
        Out,
    };

    Phase _phase = Phase::In;

public:
    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override { return "Guard"; }
};
