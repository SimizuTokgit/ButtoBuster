#pragma once
#include "AttackData.h"

// プレイヤーの技の数値
// 判定の出る時間は、アニメで剣を振っているフレームに合わせてある
namespace PlayerAttacks {
    constexpr int SLASH_COUNT = 3;

    // 通常の斬り 0〜2 段目
    const AttackData& GetSlash(int index);
    const AttackData& GetStrong();
    const AttackData& GetAntiAir();
}
