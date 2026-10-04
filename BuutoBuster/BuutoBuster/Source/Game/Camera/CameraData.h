#pragma once

// カメラの追いかけ方を決める数値 調整はこのファイルだけ見ればよい
// CameraFollow が data として 1 つ持ち、追いかける処理はここから読む
//
// 角度は度 sharpness は 1 秒あたりに目標へ近づく強さで、大きいほど早く追いつく
struct CameraData {
    // ----- 位置と画角 -----

    // 足元からこの高さを見る 肩の高さにして、背中越しに前を見る
    float lookHeight = 150.0f;

    // 見ている点からカメラまでの距離 近いほど背中に張り付いた TPS らしい見え方になる
    float distance = 430.0f;

    // 見る点を右へずらす量 プレイヤーが画面の少し左に来て、右肩越しに前が見える
    // ロックオン中は相手を真ん中に置きたいので、ずらすのをやめる
    float shoulderOffset = 55.0f;

    // 画角 重い一撃で寄るときは、ここから一瞬狭める
    float fieldOfView = 60.0f;

    // ----- 見下ろす角度 -----

    // 上を向きすぎると地面にめり込み、下を向きすぎると周りが見えない
    float basePitch = 10.0f;
    float minPitch = -10.0f;
    float maxPitch = 55.0f;

    // ----- 追いかける速さ -----

    // 上下はゆっくりにして、跳んだときに画面が揺れすぎないようにする
    float followSharpness = 10.0f;
    float verticalSharpness = 5.0f;

    // ----- 背中側への回り込み -----

    // 動いている間、背中側へ回り込む強さ 0 なら回り込まない
    // TPS のように、走る向きがいつも画面の奥になる
    float behindFollowSharpness = 2.5f;

    // 回り込むいちばん速い速さ 度/秒 速すぎると画面が振り回されて酔う
    float behindFollowMaxSpeed = 150.0f;

    // 手で回してから、回り込み始めるまで 秒 回した向きをすぐ戻されないように
    float followDelay = 0.6f;

    // この速さより遅いときは回り込まない 立ち止まって向きを変えただけで回らないように
    float followMinSpeed = 150.0f;

    // 背中とカメラの向きがこれより離れていたら回り込まない
    // カメラへ向かって走ってくるときに、画面がぐるりと半周するのを防ぐ
    float followMaxAngle = 150.0f;

    // ロックオンする相手がいないときに、背中側へ戻す速さ
    float resetSharpness = 12.0f;

    // ----- ロックオン -----

    // 相手へ向き直る速さ
    float lockTurnSharpness = 6.0f;

    // 見る点を相手の方へどれだけ寄せるか 0 でプレイヤー 1 で相手
    float lockLookWeight = 0.3f;

    // 相手が遠いほど引いて、プレイヤーと相手の両方を画面に入れる 離れた距離に掛ける割合と、引く最大
    float lockExtraDistanceRate = 0.15f;
    float lockExtraDistanceMax = 250.0f;

    // 相手がこれより近いと向きが定まらず回り続けるので、向き直らない
    float lockMinHorizontal = 120.0f;

    // ロックオンを付けたり外したりしたとき、見る点が飛ばないよう混ぜる速さ
    float lockBlendSharpness = 6.0f;

    // ----- 壁際 -----

    // 障害物からどれだけ手前に止めるか 近すぎると近クリップで壁が欠けて見える
    float obstacleMargin = 30.0f;

    // 壁に寄ったときでも、これより近づかない
    float minDistance = 120.0f;

    // 遮るものが無くなったとき、元の距離へ戻る速さ 急に戻ると画面が跳ねる
    float distanceReturnSharpness = 3.0f;

    // ----- 寄る演出 -----

    // ZoomPunch で寄るのにかける割合 残りでゆっくり戻す
    float punchInRate = 0.15f;
};
