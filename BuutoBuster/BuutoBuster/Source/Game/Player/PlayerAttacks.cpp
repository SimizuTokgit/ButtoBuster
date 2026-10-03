#include "PlayerAttacks.h"

namespace {
    AttackData CreateSlash1() {
        AttackData attack;
        attack.animationName = "Attack1";
        attack.hitStart = 4.0f;
        attack.hitEnd = 7.0f;
        attack.cancelTime = 8.5f;
        attack.damage = 12;
        attack.knockback = 150.0f;
        attack.reach = 190.0f;
        attack.arcDegree = 75.0f;
        attack.lunge = 260.0f;
        attack.hitStop = 0.045f;
        attack.hasArc = true;
        attack.arcTilt = 25.0f;
        attack.arcSwing = 1.0f;
        attack.arcColor = GetColorU8(170, 215, 255, 255);
        return attack;
    }

    AttackData CreateSlash2() {
        AttackData attack;
        attack.animationName = "Attack2";
        attack.hitStart = 4.0f;
        attack.hitEnd = 8.5f;
        attack.cancelTime = 9.5f;
        attack.damage = 14;
        attack.knockback = 180.0f;
        attack.reach = 200.0f;
        attack.arcDegree = 80.0f;
        attack.lunge = 260.0f;
        attack.hitStop = 0.05f;
        // 1 段目と逆から振り返す
        attack.hasArc = true;
        attack.arcTilt = -25.0f;
        attack.arcSwing = -1.0f;
        attack.arcColor = GetColorU8(170, 215, 255, 255);
        return attack;
    }

    // 3段目は大振りで吹き飛ばす 群れを散らしてひと息つける
    AttackData CreateSlash3() {
        AttackData attack;
        attack.animationName = "Attack3";
        attack.hitStart = 5.5f;
        attack.hitEnd = 11.5f;
        attack.cancelTime = 16.0f;
        attack.damage = 24;
        attack.reaction = HitReaction::Blow;
        attack.knockback = 650.0f;
        attack.reach = 220.0f;
        attack.arcDegree = 100.0f;
        attack.lunge = 350.0f;
        attack.hitStop = 0.09f;
        attack.shake = 6.0f;
        attack.zoomPunch = 3.0f;
        // 締めは水平に大きく薙ぎ、足元から衝撃を広げる
        attack.hasArc = true;
        attack.arcTilt = 0.0f;
        attack.arcSwing = 1.0f;
        attack.arcColor = GetColorU8(200, 235, 255, 255);
        attack.shockwaveRadius = 300.0f;
        return attack;
    }

    // 攻撃 + ガード 出が遅い代わりに重い
    AttackData CreateStrong() {
        AttackData attack;
        attack.animationName = "Attack3";
        attack.animationSpeed = 0.75f;
        attack.hitStart = 5.5f;
        attack.hitEnd = 11.5f;
        attack.cancelTime = 18.0f;
        attack.damage = 40;
        attack.reaction = HitReaction::Blow;
        attack.knockback = 900.0f;
        attack.reach = 240.0f;
        attack.arcDegree = 110.0f;
        attack.lunge = 200.0f;
        attack.hitStop = 0.12f;
        attack.shake = 10.0f;
        attack.zoomPunch = 6.0f;
        // 普通の斬りと見分けがつくよう金色にする
        attack.hasArc = true;
        attack.arcTilt = 10.0f;
        attack.arcSwing = 1.0f;
        attack.arcColor = GetColorU8(255, 205, 110, 255);
        attack.shockwaveRadius = 420.0f;
        return attack;
    }

    // 攻撃 + ジャンプ 跳びながら斬り上げて、空の Bee まで届かせる
    AttackData CreateAntiAir() {
        AttackData attack;
        attack.animationName = "Attack2";
        attack.animationSpeed = 1.1f;
        attack.hitStart = 4.0f;
        attack.hitEnd = 8.5f;
        attack.cancelTime = 0.0f;
        attack.damage = 28;
        attack.reaction = HitReaction::Blow;
        attack.knockback = 300.0f;
        attack.reach = 210.0f;
        attack.arcDegree = 90.0f;
        attack.heightMin = -40.0f;
        attack.heightMax = 560.0f;
        attack.lunge = 120.0f;
        attack.hitStop = 0.07f;
        attack.shake = 4.0f;
        // 縦に下から上へ 空の敵へ向けた振りだと分かるように
        attack.hasArc = true;
        attack.arcTilt = 90.0f;
        attack.arcSwing = 1.0f;
        attack.arcColor = GetColorU8(160, 255, 220, 255);
        return attack;
    }
}

const AttackData& PlayerAttacks::GetSlash(int index) {
    static const AttackData slashes[SLASH_COUNT] = { CreateSlash1(), CreateSlash2(), CreateSlash3() };
    if (index < 0) index = 0;
    if (index >= SLASH_COUNT) index = SLASH_COUNT - 1;
    return slashes[index];
}

const AttackData& PlayerAttacks::GetStrong() {
    static const AttackData strong = CreateStrong();
    return strong;
}

const AttackData& PlayerAttacks::GetAntiAir() {
    static const AttackData antiAir = CreateAntiAir();
    return antiAir;
}
