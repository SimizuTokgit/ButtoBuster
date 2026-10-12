#pragma once
#include "DxLib.h"

// ジャスト回避と、そのあとの反撃の色 弧 輪 文字 回避ゲージで同じ水色を使う
constexpr unsigned int JUST_DODGE_COLOR = 0x78E6FF;

// ジャスト回避が決まったことを知らせる中身
// 画面の演出 音 画面の文字は、これを受け取ってそれぞれの出し方をする
// かわした Player は、誰が受け取っているかを知らない
struct JustDodgeEvent {
    // かわした瞬間の足元と頭の上
    VECTOR position = VGet(0.0f, 0.0f, 0.0f);
    VECTOR headPosition = VGet(0.0f, 0.0f, 0.0f);
};
