#pragma once
#include "AttackData.h"

struct PlayerParams;

// プレイヤーの技の数値
// 判定の出る時間は、アニメで剣を振っているフレームに合わせてある
namespace PlayerAttacks {
    constexpr int SLASH_COUNT = 3;

    // 通常の斬り 0〜2 段目
    const AttackData& GetSlash(int index);
    const AttackData& GetStrong();
    const AttackData& GetAntiAir();

    // ヘビーアタックの溜めの段階 0 は溜めずに離したとき
    constexpr int CHARGE_LEVEL_MAX = 3;
    const AttackData& GetHeavy(int level);

    // 空中の斬り 0〜2 段目 地上と同じ振りで、1 2 段目は相手を浮かせ、3 段目で吹き飛ばす
    const AttackData& GetAirSlash(int index);

    // 空中から真下への叩きつけ 着地した場所から、この半径の敵を吹き飛ばす
    constexpr float AIR_SLAM_RADIUS = 380.0f;
    const AttackData& GetAirSlam();

    // PlayerParams の倍率 (強化で上がる分) を掛けた技にする 技を振り始めるときに通す
    AttackData ApplyRates(const AttackData& base, const PlayerParams& params);

    // ジャスト回避のあとの反撃 元の技を、必ず吹き飛ばす重い一撃にする どれだけ重くするかは PlayerParams
    AttackData CreateCounter(const AttackData& base, const PlayerParams& params);
}
