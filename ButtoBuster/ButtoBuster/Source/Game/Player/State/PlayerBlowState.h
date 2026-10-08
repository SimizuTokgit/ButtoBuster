#pragma once
#include "ICharacterState.h"
#include "ArenaWall.h"
#include "DxLib.h"

class Player;

// 吹き飛んで倒れ、起き上がるまで 壁にぶつかったら一瞬張り付いてから跳ね返る
// 跳ねる速さ 倒れている時間 起き上がったあとの無敵は PlayerData 壁で跳ね返る強さと張り付く時間は ArenaWall
class PlayerBlowState : public ICharacterState<Player> {
private:
    enum class Phase {
        Fly,
        Stick,      // 壁に張り付いている 時間が来たら跳ね返って Fly に戻る
        Down,
        GetUp,
    };

    VECTOR _knockback;
    Phase _phase = Phase::Fly;
    float _timer = 0.0f;

    // 張り付いた壁の様子と、張り付いている秒数 跳ね返すときに使う
    ArenaWall::Hit _wallHit;
    float _stickTime = 0.0f;

public:
    explicit PlayerBlowState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Blow"; }
};
