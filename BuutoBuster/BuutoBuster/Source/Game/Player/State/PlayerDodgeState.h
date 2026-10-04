#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Player;

// 回避 速さ 長さ 無敵の時間は PlayerParams
class PlayerDodgeState : public ICharacterState<Player> {
private:
    VECTOR _direction;
    float _timer = 0.0f;

public:
    explicit PlayerDodgeState(VECTOR direction) : _direction(direction) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override { return "Dodge"; }
};
