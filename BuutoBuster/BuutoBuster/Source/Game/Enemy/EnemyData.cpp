#include "EnemyData.h"

// 攻撃の判定時間は、アニメで武器を振っているフレームに合わせてある
namespace {

    // 基本の敵 数で押してくる
    // 振りかぶってから斬るまでが長いので、声を聞いてから避けられる
    EnemyData CreateGoblin() {
        EnemyData data;
        data.kind = EnemyKind::Goblin;
        data.displayName = "Goblin";
        data.folder = "Data/Character/Goblin/";
        data.modelFile = "Goblin.mv1";
        data.weaponBone = "hansocketR";
        data.weaponModel = "Data/Character/Weapon/Axe/Axe.mv1";

        // 吹っ飛ばされ値の許容値 体力があったころの体力と同じ量にしてある
        data.flinchLimit = 36.0f;
        data.blow.limit = 40.0f;
        data.walkSpeed = 200.0f;
        data.runSpeed = 420.0f;

        data.slash.animationName = "Attack1";
        data.slash.hitStart = 24.5f;
        data.slash.hitEnd = 27.5f;
        data.slash.damage = 6;
        data.slash.knockback = 280.0f;
        data.slash.reach = 170.0f;
        data.slash.arcDegree = 60.0f;
        data.slash.lunge = 60.0f;
        data.slash.hitStop = 0.03f;
        data.slash.hitSound = "Player/dmg_byAxe";
        // 敵の振りは赤くして、プレイヤーの弧と見分ける
        data.slash.hasArc = true;
        data.slash.arcTilt = 30.0f;
        data.slash.arcColor = GetColorU8(255, 90, 60, 255);

        // ときどき出す大振り 斬りを遅く振り、当たれば吹き飛ばす
        // ガードできない技にすると、頭の上の合図が赤く大きくなる 見たら避ける
        data.hasHeavy = true;
        data.heavy = data.slash;
        data.heavy.animationSpeed = 0.7f;
        data.heavy.reaction = HitReaction::Blow;
        data.heavy.damage = 10;
        data.heavy.knockback = 600.0f;
        data.heavy.canGuard = false;
        data.heavy.arcColor = GetColorU8(255, 40, 30, 255);

        data.attackRange = 150.0f;
        data.surroundRadius = 320.0f;
        data.cooldownMin = 0.8f;
        data.cooldownMax = 1.8f;

        // 賢さ 2 ガード中とホカホカの相手には大振り、こちらを向いて溜めていたら下がる
        // それ以外でも 4 回に 1 回くらい大振り
        data.intelligence = 2;
        data.reactionTime = 0.4f;
        data.heavyChance = 0.25f;

        data.cost = 0.6f;
        data.unlockPhase = 1;
        data.pickWeight = 6;

        data.soundHit = "Goblin/dmg_bySabel";
        data.soundDamage = "Goblin/VO_dmg";
        data.soundBlow = "Goblin/VO_dmgBlow";
        data.soundDead = "Goblin/VO_dead";
        return data;
    }

    // Goblin の強化版 硬くて速く、一度の振りで二回斬ってくる
    EnemyData CreateRedGoblin() {
        EnemyData data;
        data.kind = EnemyKind::RedGoblin;
        data.displayName = "RedGoblin";
        data.folder = "Data/Character/RedGoblin/";
        data.modelFile = "RedGoblin.mv1";
        data.weaponBone = "hansocketR";
        data.weaponModel = "Data/Character/Weapon/Sword/sword.mv1";
        data.scale = 1.05f;

        data.flinchLimit = 72.0f;
        data.blow.limit = 100.0f;
        data.walkSpeed = 300.0f;
        data.runSpeed = 550.0f;
        data.turnSpeed = 500.0f;
        data.weight = 1.3f;

        data.slash.animationName = "Attack1";
        data.slash.hitStart = 24.0f;
        data.slash.hitEnd = 26.5f;
        data.slash.hitStart2 = 33.5f;
        data.slash.hitEnd2 = 36.5f;
        data.slash.damage = 8;
        data.slash.knockback = 300.0f;
        data.slash.reach = 180.0f;
        data.slash.arcDegree = 70.0f;
        data.slash.lunge = 120.0f;
        data.slash.hitStop = 0.03f;
        data.slash.hitSound = "Player/dmg_bySword";
        data.slash.hasArc = true;
        data.slash.arcTilt = 20.0f;
        data.slash.arcColor = GetColorU8(255, 70, 50, 255);

        // ときどき出す大振り 斬りを遅く振り、当たれば吹き飛ばす
        data.hasHeavy = true;
        data.heavy = data.slash;
        data.heavy.animationSpeed = 0.7f;
        data.heavy.reaction = HitReaction::Blow;
        data.heavy.damage = 10;
        data.heavy.knockback = 600.0f;
        data.heavy.canGuard = false;
        data.heavy.arcColor = GetColorU8(255, 40, 30, 255);

        data.attackRange = 170.0f;
        data.surroundRadius = 340.0f;
        data.cooldownMin = 0.8f;
        data.cooldownMax = 1.8f;

        // 賢さ 3 Goblin にできることに加えて、隙に踏み込み、待つ間は背中側へ回り込む 気づくのも速い
        data.intelligence = 3;
        data.reactionTime = 0.25f;
        data.heavyChance = 0.25f;

        data.cost = 1.5f;
        data.unlockPhase = 3;
        data.pickWeight = 3;

        data.soundHit = "RedGoblin/dmg_bySabel";
        data.soundDamage = "RedGoblin/VO_dmg";
        data.soundBlow = "RedGoblin/VO_dmgBlow";
        data.soundDead = "RedGoblin/VO_dead";
        return data;
    }

    // 空を飛ぶ 地上の斬りは届かないので、対空斬りで落としてから叩く
    // 離れていれば Needle を撃ち、近ければ急降下して刺す
    EnemyData CreateBee() {
        EnemyData data;
        data.kind = EnemyKind::Bee;
        data.displayName = "Bee";
        data.folder = "Data/Character/Bee/";
        data.modelFile = "Bee.mv1";

        data.flinchLimit = 20.0f;
        data.blow.limit = 35.0f;
        data.bodyRadius = 40.0f;
        data.bodyHeight = 110.0f;
        data.bodyCenterY = 90.0f;
        data.walkSpeed = 260.0f;
        data.runSpeed = 420.0f;
        data.turnSpeed = 300.0f;
        data.isFlying = true;
        data.hoverHeight = 240.0f;
        data.weight = 0.6f;

        data.slash.animationName = "Attack2";
        data.slash.hitStart = 16.5f;
        data.slash.hitEnd = 21.0f;
        data.slash.damage = 7;
        data.slash.knockback = 200.0f;
        data.slash.reach = 190.0f;
        data.slash.arcDegree = 60.0f;
        // 上から刺すので、自分より下まで届く
        data.slash.heightMin = -400.0f;
        data.slash.heightMax = 150.0f;
        data.slash.lunge = 350.0f;
        data.slash.hitStop = 0.03f;
        data.slash.hitSound = "Player/dmg_byNeedle";

        data.canShoot = true;
        data.shootTime = 16.5f;
        data.shootDamage = 8;

        data.attackRange = 200.0f;
        data.shootRange = 750.0f;
        data.surroundRadius = 560.0f;
        data.cooldownMin = 1.6f;
        data.cooldownMax = 3.0f;

        data.cost = 1.5f;
        data.unlockPhase = 5;
        data.maxPerPhase = 3;
        data.pickWeight = 2;

        data.soundHit = "Bee/dmg_bySabel";
        data.soundDamage = "Bee/VO_damage";
        data.soundBlow = "Bee/VO_damage_B";
        data.soundDead = "Bee/VO_dead";
        return data;
    }

    // 大型 のけぞらせられない 大振りの前に予備動作の音が鳴る
    // Damage のアニメが無いことを、殴っても止まらない硬さとしてそのまま使っている
    EnemyData CreateGolem() {
        EnemyData data;
        data.kind = EnemyKind::Golem;
        data.displayName = "Golem";
        data.folder = "Data/Character/Golem/";
        data.modelFile = "golem.mv1";

        data.flinchLimit = 120.0f;
        data.blow.limit = 420.0f;
        data.bodyRadius = 70.0f;
        data.bodyHeight = 240.0f;
        data.bodyCenterY = 120.0f;
        data.walkSpeed = 130.0f;
        data.runSpeed = 130.0f;
        data.turnSpeed = 150.0f;

        data.canFlinch = false;
        data.canBlow = false;

        // 剣では止まらないが、飛んできた敵に当たると吹き飛ぶ 重いので飛ぶ距離は短く、飛べば周りを大きく巻き込む
        data.weight = 3.0f;

        data.slash.animationName = "Attack1";
        data.slash.hitStart = 25.5f;
        data.slash.hitEnd = 31.0f;
        data.slash.damage = 18;
        data.slash.reaction = HitReaction::Blow;
        data.slash.knockback = 700.0f;
        data.slash.reach = 270.0f;
        data.slash.arcDegree = 80.0f;
        data.slash.lunge = 80.0f;
        data.slash.hitStop = 0.06f;
        data.slash.shake = 8.0f;
        data.slash.hitSound = "Player/dmg_byRockKnuckle";
        data.slash.hasArc = true;
        data.slash.arcTilt = -15.0f;
        data.slash.arcSwing = -1.0f;
        data.slash.arcColor = GetColorU8(255, 120, 60, 255);

        data.hasSlashAlt = true;
        data.slashAlt = data.slash;
        data.slashAlt.animationName = "Attack2";
        data.slashAlt.hitStart = 24.5f;
        data.slashAlt.hitEnd = 31.5f;
        data.slashAlt.arcTilt = 15.0f;
        data.slashAlt.arcSwing = 1.0f;

        // 踏みつけ 周り全部に当たり、ガードできない 予備動作の音を聞いたら回避する
        data.hasHeavy = true;
        data.heavy.animationName = "Attack3";
        data.heavy.hitStart = 57.0f;
        data.heavy.hitEnd = 60.0f;
        data.heavy.damage = 26;
        data.heavy.reaction = HitReaction::Blow;
        data.heavy.knockback = 900.0f;
        data.heavy.heightMin = -60.0f;
        data.heavy.heightMax = 150.0f;
        data.heavy.canGuard = false;
        data.heavy.hitStop = 0.08f;
        data.heavy.shake = 16.0f;
        data.heavy.hitSound = "Player/dmg_byRockKnuckle";
        data.heavyAreaRadius = 380.0f;

        data.attackRange = 250.0f;
        data.heavyChance = 0.35f;
        data.surroundRadius = 420.0f;
        data.cooldownMin = 2.0f;
        data.cooldownMax = 3.5f;

        data.cost = 4.0f;
        data.unlockPhase = 8;
        data.maxPerPhase = 2;
        data.pickWeight = 1;

        data.soundHit = "Golem/dmg_bySabel";
        data.soundDead = "Golem/VO_dead";
        return data;
    }
}

const EnemyData& EnemyDatabase::Get(EnemyKind kind) {
    static const EnemyData table[] = {
        CreateGoblin(),
        CreateRedGoblin(),
        CreateBee(),
        CreateGolem(),
    };
    static_assert(sizeof(table) / sizeof(table[0]) == static_cast<size_t>(EnemyKind::Count),
        "EnemyKind を足したら表にも足すこと");

    return table[static_cast<int>(kind)];
}
