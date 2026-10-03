#pragma once
#include "ICharacterState.h"

class Player;

// 状態をまたいで使う判断
namespace PlayerActions {
    // スティックが倒されているか
    bool HasMoveInput(const InputInfo& input);

    // 待機や移動から技やガードを始める 始めたら true
    bool TryStart(Player& player, const ICharacterState<Player>* from, const InputInfo& input);
}
