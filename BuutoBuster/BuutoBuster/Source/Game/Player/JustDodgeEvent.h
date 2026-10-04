#pragma once
#include "DxLib.h"

// ジャスト回避が決まったことを知らせる中身
// スロー 音 画面の文字は、これを受け取ってそれぞれの出し方をする
// かわした Player は、誰が受け取っているかを知らない
struct JustDodgeEvent {
    // かわした瞬間の足元と頭の上
    VECTOR position = VGet(0.0f, 0.0f, 0.0f);
    VECTOR headPosition = VGet(0.0f, 0.0f, 0.0f);
};
