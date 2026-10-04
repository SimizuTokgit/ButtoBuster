#pragma once

// フェーズの進み方を決める数値 調整はこのファイルだけ見ればよい
// PhaseDirector が data として 1 つ持ち、進め方の処理と画面の表示はここから読む
//
// 敵ごとの強さと出てくる時期 (cost unlockPhase maxPerPhase pickWeight) は EnemyData.cpp に置く
struct PhaseData {
    // ----- 敵の数と強さの伸び方 -----

    // フェーズごとの強さの予算 = baseBudget + フェーズ × budgetPerPhase
    // 出てよい敵の中から、cost の合計がこの予算に収まるまで選ぶ
    float baseBudget = 3.0f;
    float budgetPerPhase = 1.6f;

    // 同時に場に出す数 = minConcurrent + フェーズ ÷ 2 (maxConcurrent まで)
    // これ以上は処理が重くなり、画面も見えなくなる
    int minConcurrent = 5;
    int maxConcurrent = 10;

    // 同時に攻撃してくる数 = 1 + (フェーズ - 1) ÷ tokenStepPhases (maxTokens まで)
    // ほかの敵は周りを回って順番を待つ
    int tokenStepPhases = 3;
    int maxTokens = 4;

    // ----- 全回復 -----

    // 何フェーズごとに全回復して休ませるか
    int healInterval = 5;

    // ----- 時間 秒 -----

    // フェーズの番号を出している時間 この間にもう敵が出始める
    float announceTime = 1.8f;

    // 全滅してから次へ進むまで
    float clearTime = 1.2f;

    // 全回復のあとの休み
    float restTime = 3.0f;

    // 敵を 1 体ずつ出す間隔
    float spawnInterval = 0.35f;

    // 倒れてから結果の画面を出すまで
    float resultDelay = 2.5f;

    // ----- 敵を出す場所 -----

    // プレイヤーからこの距離の輪の上に出す 近すぎると出た瞬間に殴られる
    float spawnDistanceMin = 800.0f;
    float spawnDistanceMax = 1200.0f;

    // 範囲の端へ戻したせいで、プレイヤーにこれより近くなったら選び直す
    float spawnMinGap = 500.0f;

    // ----- BGM -----

    // このフェーズから曲を替える BGM_stg0 → BGM_stg1 → BGM_boss
    int stage2BgmPhase = 5;
    int bossBgmPhase = 10;

    // 曲を替えるときに重ねる時間 秒
    float bgmCrossfadeTime = 2.0f;
};
