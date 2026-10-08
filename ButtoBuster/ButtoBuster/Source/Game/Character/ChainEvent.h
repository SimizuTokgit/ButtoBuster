#pragma once
#include "DxLib.h"

// 連鎖ぶっ飛ばしで起きたことを知らせる中身
// 画面の表示 音 エフェクトはこれを受け取って、それぞれの出し方をする
struct ChainEvent {
    enum class Type {
        Hit,    // 飛んでいる敵が、別の敵を巻き込んだ
        End,    // その振りで飛んだ敵が、全部止まった
    };

    Type type = Type::Hit;

    // 巻き込んだ数 Hit は今の数、End は最後の数
    int count = 0;

    // ぶつかった場所 End は最後に止まった敵の足元
    VECTOR position = VGet(0.0f, 0.0f, 0.0f);
};

// 連鎖の区切りの数 ここに届いた瞬間に、画面の表示 音 エフェクトを大きくする
// 同時に出る敵は 10 体までなので、最後の区切りはほぼ全員を巻き込んだときにする
namespace ChainMilestone {
    constexpr int FIRST = 3;
    constexpr int SECOND = 5;
    constexpr int THIRD = 8;
}
