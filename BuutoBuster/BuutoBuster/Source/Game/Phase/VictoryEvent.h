#pragma once
#include "DxLib.h"

// 最後のフェーズを越えて勝ったことを知らせる中身
// VICTORY の文字を出す瞬間に、PhaseDirector が知らせる
//
// 画面の演出や音は、これを受け取った Observer がそれぞれ出す
// 知らせる側 (PhaseDirector) は、誰が受け取っているかを知らない
struct VictoryEvent {
    // 越えたフェーズ PhaseData の finalPhase と同じ
    int phase = 0;

    // 知らせたときのプレイヤーの位置 演出を出す場所に使う
    VECTOR playerPosition = VGet(0.0f, 0.0f, 0.0f);
};
