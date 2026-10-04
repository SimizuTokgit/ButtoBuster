#pragma once
#include "AttackData.h"
#include "BlowSettings.h"

enum class EnemyKind {
    Goblin,
    RedGoblin,
    Bee,
    Golem,
    Count,
};

// 敵1種類分の数値
// どの敵も同じ Enemy クラスで動き、違いはこの数値だけにしてある
// 敵を増やすときは、ここに1つ足してアニメの一覧を用意すれば済む
struct EnemyData {
    EnemyKind kind = EnemyKind::Goblin;
    const char* displayName = "";

    // ----- 見た目 -----
    const char* folder = "";
    const char* modelFile = "";
    const char* weaponBone = nullptr;   // 武器を持たないなら nullptr
    const char* weaponModel = nullptr;
    float scale = 1.0f;

    // ----- 体 -----
    float bodyRadius = 30.0f;
    float bodyHeight = 160.0f;
    float bodyCenterY = 80.0f;

    // ----- 動き -----
    float walkSpeed = 150.0f;
    float runSpeed = 380.0f;
    float turnSpeed = 360.0f;
    bool isFlying = false;
    float hoverHeight = 0.0f;           // 地面からどれだけ浮いているか

    // ----- 崩れ方 -----
    bool canFlinch = true;              // 殴られたらのけぞるか
    bool canBlow = true;                // 吹き飛ぶか 吹き飛ぶアニメを持っているか
    float weight = 1.0f;                // 重さ 飛んできた敵とぶつかったときの飛び方に使う
    BlowSettings blow;                  // 吹っ飛ばされ値の許容値 溜まり方 減り方

    // ----- 攻撃 -----
    AttackData slash;                   // Technique::Slash で出す
    bool hasSlashAlt = false;
    AttackData slashAlt;                // 同じ指示でときどき出す別の振り
    bool hasHeavy = false;
    AttackData heavy;                   // Technique::StrongSlash で出す
    float heavyAreaRadius = 0.0f;       // 0 より大きければ自分の周り全部に当てる
    bool canShoot = false;
    float shootTime = 0.0f;             // 撃つアニメの何フレーム目で飛ばすか
    int shootDamage = 0;

    // ----- AI -----
    float attackRange = 150.0f;         // これより近づいたら振る
    float shootRange = 0.0f;            // これより近ければ撃てる
    float heavyChance = 0.0f;           // 攻撃するとき重い技を選ぶ割合
    float surroundRadius = 320.0f;      // 攻撃の番を待つ間に取る間合い
    float cooldownMin = 1.2f;           // 攻撃してから次の番を欲しがるまで
    float cooldownMax = 2.5f;

    // ----- フェーズ -----
    float cost = 1.0f;                  // フェーズの強さの予算をどれだけ使うか
    int unlockPhase = 1;                // 何フェーズ目から出るか
    int maxPerPhase = 99;
    int pickWeight = 1;                 // 選ばれやすさ

    // ----- 音 -----
    const char* soundHit = "";          // 斬られた音
    const char* soundDamage = "";       // 痛がる声
    const char* soundBlow = "";
    const char* soundDead = "";
};

namespace EnemyDatabase {
    const EnemyData& Get(EnemyKind kind);
}
