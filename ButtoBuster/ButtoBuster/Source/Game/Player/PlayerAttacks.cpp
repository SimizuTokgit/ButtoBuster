#include "PlayerAttacks.h"
#include "PlayerData.h"

namespace {
    // 反撃の弧の色 ジャスト回避と同じ水色にして、反撃だと分かるようにする
    COLOR_U8 GetCounterArcColor() {
        return GetColorU8(120, 230, 255, 255);
    }

    AttackData CreateSlash1() {
        AttackData attack;
        attack.animationName = "Attack1";
        attack.hitStart = 4.0f;
        attack.hitEnd = 7.0f;
        attack.cancelTime = 8.5f;
        attack.recovery = 3.0f;
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
        attack.recovery = 3.0f;
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
        attack.recovery = 8.0f;
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
        attack.recovery = 16.0f;
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
        attack.recovery = 24.0f;
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
            float recovery;
            COLOR_U8 arcColor;
        };

        // 吹っ飛ばし力は段階ごとに大きく伸ばし、溜めるほど遠くへ飛ぶことがはっきり分かるようにする
        // 吹っ飛ぶ角度が決まっているので (Character::LAUNCH_ANGLE)、強く飛ばすほど高く上がって遠くへ飛ぶ
        // 吹っ飛ばされ値が 0 で壁が無ければ、溜めなし 900 で約 6m、1 段目で約 12m、2 段目で約 18m、3 段目で約 24m
        // あと隙 (フレーム) も段階ごとに伸ばす 重い一撃ほど、外したときに隙をさらす
        // 色は段階が上がるほど金から赤へ寄せ、どこまで溜めたかが振りで分かるようにする
        const LevelValues values[PlayerAttacks::CHARGE_LEVEL_MAX] = {
            { 50, 1500.0f, 250.0f, 80.0f, 240.0f, 0.13f, 11.0f, 6.5f, 460.0f, 12.0f, GetColorU8(255, 215, 130, 255) },
            { 65, 2200.0f, 270.0f, 120.0f, 280.0f, 0.15f, 13.0f, 7.5f, 540.0f, 15.0f, GetColorU8(255, 185, 90, 255) },
            { 90, 3200.0f, 300.0f, 130.0f, 320.0f, 0.20f, 18.0f, 9.0f, 680.0f, 18.0f, GetColorU8(255, 130, 70, 255) },
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
        attack.recovery = value.recovery;
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

    // 空中の強攻撃 (攻撃 + ガード) 真下へ叩きつけ、着地した場所から衝撃波で周りの敵を吹き飛ばす
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

    // 必殺技の雷 当たり方は溜め斬りの 3 段目と同じ (ダメージ 90 で吹き飛ばし、吹っ飛ばし 3200)
    // 3 段目の数値を変えると、雷も一緒に変わる 雷だけ変えたいときは、ここで上書きする
    // 雷は上から落ちるのでガードできない 振らないので前にも出ない
    // 弧と衝撃波は出さない 見た目は SpecialEffectObserver が雷で見せる
    AttackData CreateSpecial() {
        AttackData attack = CreateCharged(PlayerAttacks::CHARGE_LEVEL_MAX);
        attack.canGuard = false;
        attack.lunge = 0.0f;
        attack.hasArc = false;
        attack.shockwaveRadius = 0.0f;
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

const AttackData& PlayerAttacks::GetSpecial() {
    static const AttackData special = CreateSpecial();
    return special;
}

AttackData PlayerAttacks::ApplyRates(const AttackData& base, const PlayerData& playerData) {
    AttackData attack = base;
    attack.damage = static_cast<int>(base.damage * playerData.damageRate + 0.5f);
    attack.knockback = base.knockback * playerData.knockbackRate;
    return attack;
}

AttackData PlayerAttacks::CreateCounter(const AttackData& base, const PlayerData& playerData) {
    AttackData attack = base;

    // どの技でも吹き飛ばす 吹き飛ばすなら、浮かせて留める必要は無い
    attack.reaction = HitReaction::Blow;
    attack.lift = 0.0f;

    attack.damage = static_cast<int>(base.damage * playerData.counterDamageRate);
    float knockback = base.knockback * playerData.counterKnockbackRate;
    attack.knockback = (knockback > playerData.counterKnockbackMin) ? knockback : playerData.counterKnockbackMin;

    // 手応えも重くし、ジャスト回避と同じ水色の弧で反撃だと分かるようにする
    attack.hitStop = base.hitStop + playerData.counterHitStopAdd;
    attack.shake = base.shake + playerData.counterShakeAdd;
    attack.zoomPunch = base.zoomPunch + playerData.counterZoomAdd;
    attack.arcColor = GetCounterArcColor();
    return attack;
}

AttackData PlayerAttacks::CreateCounterCombo(const AttackData& base, const PlayerData& playerData) {
    AttackData attack = base;

    // のけぞりと浮かせはそのまま残し、締めまでつなげられるようにする 弧の色で反撃の続きだと分かるように
    // 重くした分で許容値に届けば、のけぞりの技でも吹き飛ぶ (Enemy::TakeHit)
    attack.damage = static_cast<int>(base.damage * playerData.counterDamageRate);
    attack.arcColor = GetCounterArcColor();
    return attack;
}
