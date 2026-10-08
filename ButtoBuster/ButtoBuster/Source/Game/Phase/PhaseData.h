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
    // 本番では、この 2 つは選んだ難易度の値で置き換わる (Difficulty.cpp の表) ここの値はノーマルと同じ
    int tokenStepPhases = 3;
    int maxTokens = 4;

    // ----- 全回復 -----

    // 何フェーズごとに全回復して休ませるか
    int healInterval = 5;

    // ----- 勝ち -----

    // このフェーズの敵を全部倒したら勝ち 全回復や次のフェーズへは進まず、勝ちの演出と結果を出す
    // 0 にすると勝ちは無く、今までどおりどこまでも続く
    int finalPhase = 10;

    // ----- 時間 秒 -----

    // フェーズの番号を出している時間
    float announceTime = 1.8f;

    // フェーズが始まってから、最初の 1 体が出るまで 0 にすると、番号を出した瞬間から出始める
    // 前のフェーズの全滅からは clearTime + これだけ空く 息をつく間と、次の波に備える間
    float spawnStartDelay = 1.5f;

    // 全滅してから次へ進むまで
    float clearTime = 1.2f;

    // 全回復のあとの休み
    float restTime = 3.0f;

    // 敵を 1 体ずつ出す間隔
    float spawnInterval = 0.35f;

    // 倒れてから結果の画面を出すまで
    float resultDelay = 2.5f;

    // 最後のフェーズの最後の 1 体を倒してから、VICTORY の文字を出すまで
    // とどめのスローの間はゆっくり数えるので、実際は 1 秒ほど長くなる
    float victoryDelay = 0.8f;

    // VICTORY の文字を出してから結果を出すまで 結果が出たらボタンでタイトルへ戻れる
    float victoryResultDelay = 1.5f;

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
