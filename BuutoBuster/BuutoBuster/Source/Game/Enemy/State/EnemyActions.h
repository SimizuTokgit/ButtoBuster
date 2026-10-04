#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Enemy;

// 状態をまたいで使う判断
namespace EnemyActions {
    // 相手の方向 高さは見ない
    VECTOR ToTarget(const Enemy& enemy);

    // 待機や移動から攻撃を始める 始めたら true
    bool TryStartAttack(Enemy& enemy, const ICharacterState<Enemy>* from, Technique technique);
}
