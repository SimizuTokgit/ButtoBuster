#pragma once
#include "ICharacterState.h"

class Player;

// 跳ぶ 落ちる 着地する 跳ぶ強さと空中で動ける速さは PlayerData
class PlayerJumpState : public ICharacterState<Player> {
private:
    // 跳んだ直後はまだ地面に触れていることがあるので、少し待ってから着地を見る
    static constexpr float MIN_AIR_TIME = 0.15f;

    bool _hasImpulse;
    bool _isRising = true;
    bool _isLanding = false;
    float _airTime = 0.0f;

public:
    // false なら跳ばずに落ちるだけ 空中で振ったあとの着地待ちに使う
    explicit PlayerJumpState(bool hasImpulse = true) : _hasImpulse(hasImpulse) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Jump"; }
};
