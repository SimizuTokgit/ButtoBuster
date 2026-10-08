#pragma once

// 難易度 モード選択で選び、タイトルに戻るまで持ち越す
// 並びは画面のボタンの並びと同じ Count は数を数えるためだけのもの
enum class Difficulty {
    Easy,
    Normal,
    Hard,
    Count,
};

// 難易度ごとの数値 中身の表は Difficulty.cpp の先頭
struct DifficultyData {
    const char* name;           // ボタンと結果の画面に出す名前
    const char* description;    // ボタンを選んでいる間、下に出す説明

    // プレイヤーの吹っ飛ばされ値の許容値 (PlayerData の blow.limit を置き換える) 大きいほど壁を割られにくい
    float playerBlowLimit;

    // 敵の賢さに足す数 賢さ 1 以上の敵 (行動の木で動く敵) だけにかけ、1〜3 に収める
    // 賢さ 0 の敵 (ビーザトール ロックマキナ) は専用の動きなので変えない
    int intelligenceShift;

    // 同時に攻撃してくる数 = 1 + (フェーズ - 1) ÷ tokenStepPhases (maxTokens まで) PhaseData の同じ名前の値を置き換える
    int tokenStepPhases;
    int maxTokens;

    // モード選択でこのボタンを選んでいる間の背景 無ければタイトルの背景を使う
    const char* background;
};

// 選んでいる難易度 場面を切り替えると GameObject は全部消えるので、ここに置いて持ち越す
namespace GameMode {
    Difficulty Get();
    void Set(Difficulty difficulty);

    const DifficultyData& GetData();
    const DifficultyData& GetData(Difficulty difficulty);

    // 敵の賢さを、選んでいる難易度に合わせて足し引きした値
    int AdjustIntelligence(int intelligence);
}
