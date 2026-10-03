#pragma once
#include "AttackData.h"

// プレイヤーの技の数値
// アニメの判定時間は Data\Character\Player\Anim_Attack*.txt から取った
namespace PlayerAttacks {
    constexpr int SLASH_COUNT = 3;

    // 通常の斬り 0〜2 段目
    const AttackData& GetSlash(int index);
    const AttackData& GetStrong();
    const AttackData& GetAntiAir();
}
