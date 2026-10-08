#pragma once
#include "ICharacterState.h"

class Player;

// 状態をまたいで使う判断
namespace PlayerActions {
    // スティックが倒されているか
    bool HasMoveInput(const InputInfo& input);

    // 待機や移動から技やガードを始める 始めたら true
    bool TryStart(Player& player, const ICharacterState<Player>* from, const InputInfo& input);

    // 空中で技を始める 始めたら true
    // 攻撃は空中の斬り、攻撃 + ガードは真下への叩きつけ、攻撃 + ジャンプは斬り上げ
    bool TryStartAir(Player& player, const ICharacterState<Player>* from, const InputInfo& input);
}
