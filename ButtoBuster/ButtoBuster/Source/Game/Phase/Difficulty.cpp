#include "Difficulty.h"

namespace {
    // ----- 難易度の表 調整はここだけ見ればよい -----
    // 並びは Difficulty と同じ (Easy Normal Hard)
    // { 名前, 説明, 自分の許容値, 賢さの増減, 攻撃の番が増える間隔, 攻撃の番の最大, 全回復の間隔, 敵の攻撃の待ちの倍率, 背景 }
    const DifficultyData TABLE[] = {
        { "EASY",   "はじめての人向け 吹き飛ばされにくく、敵もおとなしい",
          55.0f, -1, 4, 3, 3, 1.6f, "Data/2D/ModeEasy.png" },

        { "NORMAL", "ふつうの難しさ ガードと回避を使いこなそう",
          30.0f,  0, 3, 4, 4, 1.0f, "Data/2D/ModeNormal.png" },

        { "HARD",   "腕に自信のある人向け すぐ吹き飛ばされ、敵も賢く数も多い",
          24.0f,  2, 2, 5, 5, 1.0f, "Data/2D/ModeHard.png" },
    };

    static_assert(sizeof(TABLE) / sizeof(TABLE[0]) == static_cast<int>(Difficulty::Count),
        "Difficulty を足したら TABLE にも同じ順で足すこと");

    // 賢さの範囲 行動の木で動く敵は 1〜5 (EnemyAI.h の枝の一覧)
    constexpr int MIN_INTELLIGENCE = 1;
    constexpr int MAX_INTELLIGENCE = 5;

    Difficulty g_difficulty = Difficulty::Normal;
}

Difficulty GameMode::Get() {
    return g_difficulty;
}

void GameMode::Set(Difficulty difficulty) {
    if (difficulty < Difficulty::Easy || difficulty >= Difficulty::Count) return;
    g_difficulty = difficulty;
}

const DifficultyData& GameMode::GetData() {
    return GetData(g_difficulty);
}

const DifficultyData& GameMode::GetData(Difficulty difficulty) {
    int index = static_cast<int>(difficulty);
    if (index < 0 || index >= static_cast<int>(Difficulty::Count)) index = static_cast<int>(Difficulty::Normal);
    return TABLE[index];
}

int GameMode::AdjustIntelligence(int intelligence) {
    // 賢さ 0 の敵は行動の木を使わない専用の動きなので、そのまま
    if (intelligence < MIN_INTELLIGENCE) return intelligence;

    int adjusted = intelligence + GetData().intelligenceShift;
    if (adjusted < MIN_INTELLIGENCE) adjusted = MIN_INTELLIGENCE;
    if (adjusted > MAX_INTELLIGENCE) adjusted = MAX_INTELLIGENCE;
    return adjusted;
}
