#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Player;

// 壁を割られて場外へ飛んでいき、倒れる ゲームの負け
// 倒された体はもう壁に止められない 負けたあとの流れは PhaseDirector が進める
class PlayerDeadState : public ICharacterState<Player> {
private:
    // 割った勢いで上へ跳ねる速さ 場外へ大きく飛んでいくのが見えるように
    static constexpr float JUMP_SPEED = 700.0f;

    VECTOR _knockback;
    bool _isDown = false;

public:
    // knockback は場外へ飛んでいく向きと速さ
    explicit PlayerDeadState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Dead"; }
};
