#include "Difficulty.h"

namespace {
    // ----- 難易度の表 調整はここだけ見ればよい -----
    // 1 つの難易度を 1 つの関数で作り、値には名前を付けて書く (数字を並べただけだと、どれが何か分からないので)

    // はじめての人向け 吹き飛ばされにくく、敵もおとなしい
    DifficultyData CreateEasy() {
        DifficultyData data;
        data.name = "EASY";
        data.description = "はじめての人向け 吹き飛ばされにくく、敵もおとなしい";
        data.background = "Data/2D/ModeEasy.png";

        data.playerBlowLimit = 55.0f;     // 自分の許容値 大きいほど壁を割られにくい
        data.intelligenceShift = -1;      // 敵の賢さを 1 下げる (グレゴブリン 2 → 1)
        data.tokenStepPhases = 4;         // 同時に攻撃してくる数は、4 フェーズごとに 1 増える
        data.maxTokens = 3;               // 同時に攻撃してくる数は 3 まで
        data.healInterval = 3;            // 3 フェーズごとに全回復 (3 6 9 の 3 回)
        data.enemyCooldownRate = 1.6f;    // 敵の攻撃の待ちを 1.6 倍 (攻撃がまばら)
        return data;
    }

    // ふつうの難しさ
    DifficultyData CreateNormal() {
        DifficultyData data;
        data.name = "NORMAL";
        data.description = "ふつうの難しさ ガードと回避を使いこなそう";
        data.background = "Data/2D/ModeNormal.png";

        data.playerBlowLimit = 30.0f;
        data.intelligenceShift = 0;       // 敵の賢さはそのまま
        data.tokenStepPhases = 3;
        data.maxTokens = 4;
        data.healInterval = 4;            // 4 8 の 2 回
        data.enemyCooldownRate = 1.0f;    // 敵の攻撃の待ちはそのまま
        return data;
    }

    // 腕に自信のある人向け
    DifficultyData CreateHard() {
        DifficultyData data;
        data.name = "HARD";
        data.description = "腕に自信のある人向け すぐ吹き飛ばされ、敵も賢く数も多い";
        data.background = "Data/2D/ModeHard.png";

        data.playerBlowLimit = 24.0f;
        data.intelligenceShift = 2;       // 敵の賢さを 2 上げる (グレゴブリン 2 → 4)
        data.tokenStepPhases = 2;
        data.maxTokens = 5;
        data.healInterval = 5;            // 5 の 1 回
        data.enemyCooldownRate = 1.0f;
        return data;
    }

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

    // 並びは Difficulty と同じ (Easy Normal Hard) 最初に呼ばれたときに 1 回だけ作る
    static const DifficultyData table[] = {
        CreateEasy(),
        CreateNormal(),
        CreateHard(),
    };
    static_assert(sizeof(table) / sizeof(table[0]) == static_cast<size_t>(Difficulty::Count),
        "Difficulty を足したら、ここにも同じ順で足すこと");

    return table[index];
}

int GameMode::AdjustIntelligence(int intelligence) {
    // 賢さ 0 の敵は行動の木を使わない専用の動きなので、そのまま
    if (intelligence < MIN_INTELLIGENCE) return intelligence;

    int adjusted = intelligence + GetData().intelligenceShift;
    if (adjusted < MIN_INTELLIGENCE) adjusted = MIN_INTELLIGENCE;
    if (adjusted > MAX_INTELLIGENCE) adjusted = MAX_INTELLIGENCE;
    return adjusted;
}
