#pragma once
#include "ICharacterState.h"
#include "PlayerChargeEvent.h"

class Player;

// 強攻撃 (ヘビーアタック) の溜め
// 決めた姿勢でアニメを止め、押している長さで段階を上げる
// 溜めている間はその場から動けず、向きだけ変えられる
// 離したら、溜まった段階のヘビーアタックを振る 止めた姿勢が振りのアニメなら、その続きから振る
// 段階が上がる時間と止める姿勢は PlayerData
//
// 段階が変わったことは Player の Subject で知らせる
// 音やエフェクトはそれを受け取る側が鳴らすので、ここには書かない
class PlayerChargeState : public ICharacterState<Player> {
private:
    float _chargeTime = 0.0f;
    int _level = 0;
    bool _hasReleased = false;

public:
    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override;

private:
    void HoldPose(Player& player) const;
    void Release(Player& player);
    void Notify(Player& player, PlayerChargeEvent::Type type) const;
};
