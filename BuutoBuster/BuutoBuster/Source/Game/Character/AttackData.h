#pragma once
#include "HitInfo.h"

// 近接攻撃1つ分の数値
// 時間はすべてアニメのフレーム 30fps で数える
// 判定の出る時間は Data の Anim_*.txt の AttackStart と AttackEnd から取った
struct AttackData {
    const char* animationName = "";
    float animationSpeed = 1.0f;

    // 判定が出ている時間 2回目は RedGoblin の二段斬り用 使わないなら負
    float hitStart = 0.0f;
    float hitEnd = 0.0f;
    float hitStart2 = -1.0f;
    float hitEnd2 = -1.0f;

    // これ以降は次の行動を受け付ける 0 ならアニメが終わるまで待つ
    float cancelTime = 0.0f;

    int damage = 10;
    HitReaction reaction = HitReaction::Flinch;
    float knockback = 200.0f;

    // 自分の正面の扇形で当てる
    float reach = 150.0f;
    float arcDegree = 70.0f;

    // 自分の足元から見て、どの高さまで届くか
    // 地上の技は空の Bee に届かないようにしてある
    float heightMin = -60.0f;
    float heightMax = 200.0f;

    // 振りながら前に出る速さ
    float lunge = 0.0f;

    bool canGuard = true;

    // 当てたときの手応え
    float hitStop = 0.05f;
    float shake = 0.0f;

    // 当てたときに一瞬寄る画角 度 0 なら寄らない
    float zoomPunch = 0.0f;

    const char* hitSound = "";

    // ----- 見た目 -----

    // 振ったときに出す斬撃の弧
    bool hasArc = false;

    // 0 で水平 90 で縦 振り下ろしや斬り上げを傾きで見せる
    float arcTilt = 0.0f;

    // 1 で左から右へ -1 で右から左へ 縦のときは 1 で下から上へ
    float arcSwing = 1.0f;

    COLOR_U8 arcColor = GetColorU8(255, 255, 255, 255);

    // 0 より大きければ、振り下ろした瞬間に足元から衝撃波を広げる
    float shockwaveRadius = 0.0f;
};
