#pragma once
#include "DxLib.h"

// ヘビーアタックの溜めで起きたことを知らせる中身
// 音やエフェクトはこれを受け取って鳴らしたり光らせたりする
// 溜めている Player は、誰が受け取っているかを知らない
struct PlayerChargeEvent {
    enum class Type {
        Start,      // 溜め始めた
        LevelUp,    // 段階が上がった
        Release,    // 離して振った
        Cancel,     // 被弾や回避で振らずにやめた
    };

    Type type = Type::Start;

    // 今の段階 0 はまだ溜まっていない
    int level = 0;
    int maxLevel = 0;

    VECTOR position = VGet(0.0f, 0.0f, 0.0f);
    VECTOR headPosition = VGet(0.0f, 0.0f, 0.0f);
};
