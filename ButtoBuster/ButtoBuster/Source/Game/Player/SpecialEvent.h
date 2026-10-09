#pragma once
#include "DxLib.h"
#include <vector>

// 必殺技で起きたことを知らせる中身
// 画面の演出と音は、これを受け取ってそれぞれの出し方をする
// 撃った Player は、誰が受け取っているかを知らない
struct SpecialEvent {
    enum class Type {
        Ready,      // ゲージが満タンになった
        Call,       // 剣を掲げて雷を呼び始めた
        Strike,     // 雷が落ちた
    };

    Type type = Type::Ready;

    // プレイヤーの足元
    VECTOR position = VGet(0.0f, 0.0f, 0.0f);

    // 雷が落ちるまでの秒数 Call のときだけ入る 呼んでいる間の演出の長さに使う
    float callTime = 0.0f;

    // 雷が落ちた敵の足元 Strike のときだけ入る
    std::vector<VECTOR> targets;
};
