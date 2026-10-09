#pragma once
#include "AttackData.h"
#include <memory>
#include <vector>

class Character;
class BlowChain;

// 近接攻撃の当たり判定
//
// 自分の正面の扇形に入っていて、高さの範囲も合う相手に当てる
// 無双の手触りにしたいので、範囲にいる相手は何体でもまとめて当たる
// 同じ振りで2回当てないよう、当てた相手を hitList に覚えておく
namespace CombatSystem {

    // 今回新しく当てた数を返す
    // chain を渡すと、吹き飛ばした相手はその連鎖の砲弾になる
    int ApplyMelee(Character& attacker, const AttackData& attack, std::vector<Character*>& hitList,
        const std::shared_ptr<BlowChain>& chain = nullptr);

    // 自分を中心にした円 Golem の踏みつけと、プレイヤーの空中からの叩きつけに使う
    int ApplyArea(Character& attacker, float radius, const AttackData& attack, std::vector<Character*>& hitList,
        const std::shared_ptr<BlowChain>& chain = nullptr);

    // 場にいる相手全員 距離も高さも問わない プレイヤーの必殺技 (雷) に使う
    // 押す向きは、自分から相手へ向かう向き
    int ApplyAll(Character& attacker, const AttackData& attack, std::vector<Character*>& hitList,
        const std::shared_ptr<BlowChain>& chain = nullptr);
}
