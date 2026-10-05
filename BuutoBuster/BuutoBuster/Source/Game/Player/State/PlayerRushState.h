#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Player;

// ジャスト回避が決まったあと、かわした敵の目の前まで一気に寄る
// 寄っている間に押した技は、着いたらすぐ出す 寄る速さと止まる所は PlayerData の counterRush で始まる値
class PlayerRushState : public ICharacterState<Player> {
private:
    // 敵の足元と体の太さ 寄り始めたときのものを使う 寄っている間に敵が消えても困らないように
    VECTOR _targetPosition;
    float _targetRadius;

    // 止まる所 敵の手前
    VECTOR _destination = VGet(0.0f, 0.0f, 0.0f);
    float _timer = 0.0f;

    // 寄っている間に押された技 着いたら出す
    Technique _queued = Technique::None;

public:
    PlayerRushState(VECTOR targetPosition, float targetRadius)
        : _targetPosition(targetPosition)
        , _targetRadius(targetRadius) {
    }

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override { return "Rush"; }
};
