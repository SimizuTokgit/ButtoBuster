#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Player;

// 吹き飛んで倒れ、起き上がるまで
// 跳ねる速さ 倒れている時間 起き上がったあとの無敵は PlayerData
class PlayerBlowState : public ICharacterState<Player> {
private:
    enum class Phase {
        Fly,
        Down,
        GetUp,
    };

    VECTOR _knockback;
    Phase _phase = Phase::Fly;
    float _timer = 0.0f;

public:
    explicit PlayerBlowState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Blow"; }
};
