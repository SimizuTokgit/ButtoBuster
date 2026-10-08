#pragma once
#include "DxLib.h"

// ボタンの組み合わせで決まる技
// 人の操作では PlayerController が同時押しを見て決め、AI は自分で選んで入れる
enum class Technique {
    None,
    Slash,          // 攻撃
    AntiAir,        // 攻撃 + ジャンプ
    StrongSlash,    // 攻撃 + ガード プレイヤーは攻撃を押し続けると溜める
    Dodge,          // ガード + ジャンプ
    Jump,           // ジャンプ
    Shoot,          // 敵だけが使う 飛び道具
};

// キャラへの入力
// 人が操作するときは PlayerController AI が動かすときは EnemyAI がこれを作る
// キャラ本体はどちらが作ったかを知らない
struct InputInfo {
    // 進みたい方向 ワールドのXZ 長さは0〜1
    VECTOR move = VGet(0.0f, 0.0f, 0.0f);

    // 向きたい先 敵の AI が相手を見続けたいときに入れる 使わないなら長さ0 プレイヤーは使わない
    VECTOR look = VGet(0.0f, 0.0f, 0.0f);

    // 押している間ずっと true
    bool isGuardHeld = false;

    // 強攻撃の溜めを続けるボタンを押している間 true
    bool isHeavyHeld = false;

    // 決まった瞬間の1フレームだけ入る
    Technique technique = Technique::None;
};
