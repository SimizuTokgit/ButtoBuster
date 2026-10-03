#pragma once
#include "AttackData.h"
#include <vector>

class Character;

// 近接攻撃の当たり判定
//
// 自分の正面の扇形に入っていて、高さの範囲も合う相手に当てる
// 無双の手触りにしたいので、範囲にいる相手は何体でもまとめて当たる
// 同じ振りで2回当てないよう、当てた相手を hitList に覚えておく
namespace CombatSystem {

    // 今回新しく当てた数を返す
    int ApplyMelee(Character& attacker, const AttackData& attack, std::vector<Character*>& hitList);

    // 自分を中心にした円 Golem の踏みつけに使う
    int ApplyArea(Character& attacker, float radius, const AttackData& attack, std::vector<Character*>& hitList);
}
