#pragma once
#include "Difficulty.h"

// 最高記録の保存
// 難易度ごとの到達フェーズだけなので、テキストに数字を難易度の数だけ 1 行ずつ書く (Easy Normal Hard の順)
// 難易度を作る前の 1 行だけのファイルは、ノーマルの記録として読む
namespace SaveData {
    int LoadBestPhase(Difficulty difficulty);
    void SaveBestPhase(Difficulty difficulty, int phase);
}
