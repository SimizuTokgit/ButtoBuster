#pragma once
#include "PlayerAttacks.h"
#include "BlowSettings.h"
#include <string>

// プレイヤーの動きと手応えを決める数値 調整はこのファイルだけ見ればよい
// 敵の EnemyData と同じ形 Unity の ScriptableObject のように、数値の入れ物と、それを読む処理を分けてある
//
// Player が 1 つ持ち、各状態は player.data から読む
// 遊んでいる途中でも書き換えられるので、強化はこの値を変える
// 新しいゲームでは Player が作り直されるので、ここに書いた初期値に戻る
//
// 技ごとの数値 (ダメージ 届く距離 判定のフレーム) は PlayerAttacks.cpp に置く
// アニメのフレームに結び付いた数は、アニメを差し替えたときに一緒に直すので各状態に残す
struct PlayerData {
    // ----- 体 -----

    // 被弾してから次に食らうまでの猶予 囲まれて起き上がれないまま殴られ続けるのを防ぐ
    float hurtInvincibleTime = 0.6f;

    // 吹き飛ばされたときに上へ跳ねる最低の速さと、倒れている時間 秒
    // 強く飛ばされたときは、吹っ飛ぶ角度 (Character::LAUNCH_ANGLE) になるまで高く跳ねる
    float blownJumpSpeed = 420.0f;
    float blownDownTime = 0.45f;

    // 吹き飛ばされて起き上がったあとに残す無敵 秒
    float getUpInvincibleTime = 0.4f;

    // 吹っ飛ばされ値の許容値 溜まり方 減り方 中身は BlowSettings.h
    // 体力はない 許容値に届いた状態で壁にぶつかると、壁を割られて負け
    BlowSettings blow;

    // ----- 移動 -----

    float moveSpeed = 600.0f;

    // 向きを変える速さ 度/秒
    float turnSpeed = 900.0f;

    // 溜めとガードの間に向きを変える速さ turnSpeed に掛ける割合
    float holdTurnRate = 0.5f;

    // ----- ジャンプ -----

    // 760 で約 3m 跳ぶ
    float jumpSpeed = 760.0f;

    // 空中で動ける速さ moveSpeed に掛ける割合
    float airMoveRate = 0.85f;

    // ----- 回避 -----

    float dodgeSpeed = 1500.0f;

    // 走り抜ける時間と、そのあと次の行動を受け付けるまで 秒
    float dodgeTime = 0.26f;
    float dodgeRecoveryTime = 0.1f;

    // 回避の無敵 秒 走り抜けている間は全部かわせる
    float dodgeInvincibleTime = 0.3f;

    // ----- ジャスト回避 -----

    // 回避を始めてからこの秒数のうちに攻撃が来たら、ジャスト回避になる
    float justDodgeWindow = 0.15f;

    // 決まったあとの無敵 秒 スローの間に続けて来た攻撃も受けない
    float justDodgeInvincibleTime = 0.6f;

    // 次の攻撃が反撃になる時間 秒
    float counterTime = 1.5f;

    // 反撃にしたときの重さ ダメージと吹っ飛ばしは元の技の何倍か
    float counterDamageRate = 1.5f;
    float counterKnockbackRate = 1.5f;

    // 軽い斬りの反撃でも、群れを崩せるだけは飛ばす
    float counterKnockbackMin = 900.0f;

    // 反撃の手応えに足す分 ヒットストップ 秒 / 画面の揺れ / 寄る角度 度
    float counterHitStopAdd = 0.06f;
    float counterShakeAdd = 6.0f;
    float counterZoomAdd = 4.0f;

    // ----- ガード -----

    // 正面から左右にどこまでの攻撃を防げるか 1 で真正面だけ
    float guardDot = 0.2f;

    // ガードで受けたときに押される強さ 攻撃の吹っ飛ばしに掛ける割合
    float guardPushRate = 0.35f;

    // ----- 攻撃 -----

    // すべての技のダメージと吹っ飛ばしに掛ける倍率 強化で上げる
    float damageRate = 1.0f;
    float knockbackRate = 1.0f;

    // 振り始めに近くの敵へ向きを吸い付ける範囲と、正面から左右に何度までの敵を見るか (0.5 で 60 度)
    float aimRadius = 420.0f;
    float aimDot = 0.5f;

    // 対空斬り (攻撃 + ジャンプ) で跳ぶ速さ
    float antiAirJumpSpeed = 620.0f;

    // コンボが途切れるまでの時間 秒
    float comboKeepTime = 2.5f;

    // ----- 空中 -----

    // 着地までに空中で浮き直せる回数 空中の斬り 1 回分
    int airHangCount = 3;

    // 空中で振るときに浮き直す速さと、振っている間に落ちる速さの上限
    float airHangSpeed = 150.0f;
    float airFallSpeed = 120.0f;

    // 空中からの叩きつけで落ちる速さと、着地してから動けるまで 秒
    float airSlamDiveSpeed = 2200.0f;
    float airSlamRecoveryTime = 0.35f;

    // ----- 溜め (ヘビーアタック) -----

    // 押し始めてからこの秒数で 1 2 3 段目に上がる 段階の数 (CHARGE_LEVEL_MAX) と同じ数だけ書く
    // 1 段目より前に離したら、溜めていない普通のヘビーアタックになる
    float chargeLevelUpTimes[PlayerAttacks::CHARGE_LEVEL_MAX] = { 0.35f, 0.85f, 1.4f };

    // 溜めている間に止める姿勢 アニメの名前と、止めるフレーム
    // ゲーム中に P と , . で探せる 決まった値をここに書くと、普段の溜めの姿勢になる
    std::string chargePoseAnimation = "Attack3";
    float chargePoseTime = 5.5f;
};
