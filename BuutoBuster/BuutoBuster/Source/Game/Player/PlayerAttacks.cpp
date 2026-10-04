#include "PlayerAttacks.h"

namespace {
    // 反撃にしたときの重さ 元の技の何倍か
    constexpr float COUNTER_DAMAGE_RATE = 1.5f;
    constexpr float COUNTER_KNOCKBACK_RATE = 1.5f;

    // 軽い斬りの反撃でも、群れを崩せるだけは飛ばす
    constexpr float COUNTER_KNOCKBACK_MIN = 900.0f;

    // 反撃の手応えに足す分
    constexpr float COUNTER_HIT_STOP_ADD = 0.06f;
    constexpr float COUNTER_SHAKE_ADD = 6.0f;
    constexpr float COUNTER_ZOOM_ADD = 4.0f;

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

    // 溜めたヘビーアタック 段階が上がるほど重く、遠くまで吹き飛ばす
    // 振りかぶりは溜めている間に済んでいるので、離したら普段の速さで振り抜く
    AttackData CreateCharged(int level) {
        struct LevelValues {
            int damage;
            float knockback;
            float reach;
            float arcDegree;
            float lunge;
            float hitStop;
            float shake;
            float zoomPunch;
            float shockwaveRadius;
            COLOR_U8 arcColor;
        };

        // 吹っ飛ばし力は段階ごとに大きく伸ばし、溜めるほど遠くへ飛ぶことがはっきり分かるようにする
        // 敵が飛び上がる速さは決まっているので、飛ぶ距離はほぼ吹っ飛ばし力に比例する
        // 溜めなし 900 で約 4m、1 段目で約 7m、2 段目で約 10m、3 段目で約 15m
        // 色は段階が上がるほど金から赤へ寄せ、どこまで溜めたかが振りで分かるようにする
        const LevelValues values[PlayerAttacks::CHARGE_LEVEL_MAX] = {
            { 50, 1500.0f, 250.0f, 115.0f, 240.0f, 0.13f, 11.0f, 6.5f, 460.0f, GetColorU8(255, 215, 130, 255) },
            { 65, 2200.0f, 270.0f, 120.0f, 280.0f, 0.15f, 13.0f, 7.5f, 540.0f, GetColorU8(255, 185, 90, 255) },
            { 90, 3200.0f, 300.0f, 130.0f, 320.0f, 0.20f, 18.0f, 9.0f, 680.0f, GetColorU8(255, 130, 70, 255) },
        };
        const LevelValues& value = values[level - 1];

        AttackData attack = CreateStrong();
        attack.animationSpeed = 1.0f;
        attack.damage = value.damage;
        attack.knockback = value.knockback;
        attack.reach = value.reach;
        attack.arcDegree = value.arcDegree;
        attack.lunge = value.lunge;
        attack.hitStop = value.hitStop;
        attack.shake = value.shake;
        attack.zoomPunch = value.zoomPunch;
        attack.shockwaveRadius = value.shockwaveRadius;
        attack.arcColor = value.arcColor;
        return attack;
    }

    // 空中の斬り 地上の 3 段と同じ振りを使う
    // 1 2 段目は相手を真上へ浮かせて宙に留め、3 段目はいつもどおり吹き飛ばす
    // 跳んだ高さからでも地上の敵に届くよう、届く高さを足元より下へ広げる
    // 宙で前に出すぎると浮かせた相手を追い越すので、前に出る速さは地上より抑える
    AttackData CreateAirSlash(AttackData attack) {
        attack.heightMin = -150.0f;
        attack.lunge = 120.0f;

        // 空中では地面を叩かないので、足元の衝撃波は出さない
        attack.shockwaveRadius = 0.0f;

        if (attack.reaction == HitReaction::Flinch) {
            attack.knockback = 60.0f;
            attack.lift = 500.0f;
        }
        return attack;
    }

    // 空中の△ 真下へ叩きつけ、着地した場所から衝撃波で周りの敵を吹き飛ばす
    // 振りかぶりはヘビーアタックと同じ大振りを使う
    AttackData CreateAirSlam() {
        AttackData attack;
        attack.animationName = "Attack3";
        attack.damage = 30;
        attack.reaction = HitReaction::Blow;
        attack.knockback = 1100.0f;
        attack.hitStop = 0.12f;
        attack.shake = 14.0f;
        attack.zoomPunch = 6.0f;
        attack.arcColor = GetColorU8(255, 190, 110, 255);
        attack.shockwaveRadius = PlayerAttacks::AIR_SLAM_RADIUS;
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

const AttackData& PlayerAttacks::GetHeavy(int level) {
    // 溜めずに離したときは、溜めのない強斬りの数値で振る
    if (level <= 0) return GetStrong();
    if (level > CHARGE_LEVEL_MAX) level = CHARGE_LEVEL_MAX;

    static const AttackData charged[CHARGE_LEVEL_MAX] = { CreateCharged(1), CreateCharged(2), CreateCharged(3) };
    return charged[level - 1];
}

const AttackData& PlayerAttacks::GetAirSlash(int index) {
    static const AttackData slashes[SLASH_COUNT] = {
        CreateAirSlash(CreateSlash1()), CreateAirSlash(CreateSlash2()), CreateAirSlash(CreateSlash3()),
    };
    if (index < 0) index = 0;
    if (index >= SLASH_COUNT) index = SLASH_COUNT - 1;
    return slashes[index];
}

const AttackData& PlayerAttacks::GetAirSlam() {
    static const AttackData slam = CreateAirSlam();
    return slam;
}

AttackData PlayerAttacks::CreateCounter(const AttackData& base) {
    AttackData attack = base;

    // どの技でも吹き飛ばす 吹き飛ばすなら、浮かせて留める必要は無い
    attack.reaction = HitReaction::Blow;
    attack.lift = 0.0f;

    attack.damage = static_cast<int>(base.damage * COUNTER_DAMAGE_RATE);
    float knockback = base.knockback * COUNTER_KNOCKBACK_RATE;
    attack.knockback = (knockback > COUNTER_KNOCKBACK_MIN) ? knockback : COUNTER_KNOCKBACK_MIN;

    // 手応えも重くし、ジャスト回避と同じ水色の弧で反撃だと分かるようにする
    attack.hitStop = base.hitStop + COUNTER_HIT_STOP_ADD;
    attack.shake = base.shake + COUNTER_SHAKE_ADD;
    attack.zoomPunch = base.zoomPunch + COUNTER_ZOOM_ADD;
    attack.arcColor = GetColorU8(120, 230, 255, 255);
    return attack;
}
