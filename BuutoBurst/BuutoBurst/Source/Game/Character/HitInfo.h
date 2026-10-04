#pragma once
#include "DxLib.h"
#include <memory>

class Character;
class BlowChain;

// 攻撃を受けたときの崩れ方
enum class HitReaction {
    Flinch,     // その場でのけぞる
    Blow,       // 吹き飛んで倒れる
};

// 1回の攻撃が相手に伝える内容
struct HitInfo {
    Character* attacker = nullptr;

    // 攻撃が飛んできた場所 ガードの向きの判定に使う
    // 飛び道具は撃った本人がもう倒れていることがあるので、本人の位置ではなくこれを見る
    VECTOR sourcePosition = VGet(0.0f, 0.0f, 0.0f);

    int damage = 0;
    HitReaction reaction = HitReaction::Flinch;

    // 相手を押す向きと強さ 水平
    VECTOR knockback = VGet(0.0f, 0.0f, 0.0f);

    // のけぞった相手を真上へ浮かせる速さ 0 なら浮かせない
    float lift = 0.0f;

    bool canGuard = true;

    // 当たったときに鳴らす音 武器によって変わる
    const char* hitSound = "";

    // 吹き飛ばした相手を入れる連鎖 プレイヤーの振りと、飛んでいる敵が持っている
    // 敵が敵に当てたときも同じものを渡すので、1 回の振りから広がった分をまとめて数えられる
    std::shared_ptr<BlowChain> chain;

    // 飛んできた敵に当たったか 重くて殴っても止まらない敵も、これなら吹き飛ぶ
    bool isFromProjectile = false;
};

// 当てた結果
enum class HitResult {
    Ignored,    // 無敵や倒れた後で効かなかった
    Guarded,
    Hit,
    Killed,
};
