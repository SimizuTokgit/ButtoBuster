#pragma once

// 難易度 モード選択で選び、タイトルに戻るまで持ち越す
// 並びは画面のボタンの並びと同じ Count は数を数えるためだけのもの
enum class Difficulty {
    Easy,
    Normal,
    Hard,
    Count,
};

// 難易度ごとの数値 中身は Difficulty.cpp の先頭 (CreateEasy / CreateNormal / CreateHard)
// 初めの値は NORMAL と同じ 書き忘れた値は NORMAL の値になる
struct DifficultyData {
    const char* name = "";          // ボタンと結果の画面に出す名前
    const char* description = "";   // ボタンを選んでいる間、下に出す説明

    // プレイヤーの吹っ飛ばされ値の許容値 (PlayerData の blow.limit を置き換える) 大きいほど壁を割られにくい
    float playerBlowLimit = 30.0f;

    // 敵の賢さに足す数 賢さ 1 以上の敵 (行動の木で動く敵) だけにかけ、1〜5 に収める
    // 賢さ 0 の敵 (ビーザトール ロックマキナ) は専用の動きなので変えない
    int intelligenceShift = 0;

    // 同時に攻撃してくる数 = 1 + (フェーズ - 1) ÷ tokenStepPhases (maxTokens まで) PhaseData の同じ名前の値を置き換える
    int tokenStepPhases = 3;
    int maxTokens = 4;

    // 何フェーズごとに全回復するか PhaseData の healInterval を置き換える
    // 勝ちはフェーズ 10 なので、3 なら 3 6 9 の 3 回、4 なら 4 8 の 2 回、5 なら 5 の 1 回
    int healInterval = 4;

    // 敵が攻撃してから次の攻撃の番を欲しがるまでの待ち (EnemyData の cooldownMin / Max) に掛ける数
    // 大きいほど攻撃がまばらになる
    float enemyCooldownRate = 1.0f;

    // モード選択でこのボタンを選んでいる間の背景 無ければタイトルの背景を使う
    const char* background = "";
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
