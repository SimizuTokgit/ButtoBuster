#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Player;

class PlayerDodgeState : public ICharacterState<Player> {
private:
    static constexpr float DASH_SPEED = 1500.0f;
    static constexpr float DASH_TIME = 0.26f;
    static constexpr float RECOVERY_TIME = 0.1f;

    // 回避の無敵 走り抜けている間は全部かわせる
    static constexpr float INVINCIBLE_TIME = 0.3f;

    VECTOR _direction;
    float _timer = 0.0f;

public:
    explicit PlayerDodgeState(VECTOR direction) : _direction(direction) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override { return "Dodge"; }
};
