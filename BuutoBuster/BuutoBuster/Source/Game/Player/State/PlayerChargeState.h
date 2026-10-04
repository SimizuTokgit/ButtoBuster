#pragma once
#include "ICharacterState.h"
#include "PlayerChargeEvent.h"

class Player;

// ヘビーアタックの溜め
// 振りかぶった姿勢でアニメを止め、押している長さで段階を上げる
// 離したら、溜まった段階のヘビーアタックを止めた姿勢の続きから振る
//
// 段階が変わったことは Player の Subject で知らせる
// 音やエフェクトはそれを受け取る側が鳴らすので、ここには書かない
class PlayerChargeState : public ICharacterState<Player> {
private:
    // 振りかぶりきった姿勢のアニメの時間 ここで止めて溜める
    static constexpr float HOLD_POSE_TIME = 3.5f;

    // 溜めながら歩く速さ 普段の何倍か
    static constexpr float WALK_RATE = 0.25f;

    float _chargeTime = 0.0f;
    int _level = 0;
    bool _isHoldingPose = false;
    bool _hasReleased = false;

public:
    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override;

private:
    void Release(Player& player);
    void Notify(Player& player, PlayerChargeEvent::Type type) const;
};
