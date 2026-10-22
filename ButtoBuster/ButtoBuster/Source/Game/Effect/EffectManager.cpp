#include "EffectManager.h"
#include "GameObject.h"
#include "Transform.h"
#include "ParticleSystem.h"
#include "ScreenFlash.h"
#include "CameraFollow.h"
#include "StageBuilder.h"
#include "ArenaBoundary.h"
#include "Time.h"

namespace {
    const VECTOR UP = { 0.0f, 1.0f, 0.0f };

    // 押された向きの火花を、少しだけ上へ跳ねさせる
    constexpr float SPARK_LIFT = 0.35f;

    // 割れた壁のかけらは、火花より少し上向きに散らす
    constexpr float SHARD_LIFT = 0.4f;

    // 粒を散らす角度 (度) は、向けた向きから広げる角度
    // 向きが無いときは全方向へ散らす (around の数)

    // 斬ったとき (PlayHit) 線の火花と丸い火花
    struct HitLook {
        float streakSpread;
        int streakCount;
        float sparkSpread;
        int sparkCount;
        int aroundStreakCount;
        int aroundSparkCount;
    };
    const HitLook HIT_LOOK = { 35.0f, 9, 60.0f, 6, 8, 8 };

    // ガードしたとき (PlayGuard)
    struct GuardLook {
        float spread;
        int count;
        int aroundCount;
    };
    const GuardLook GUARD_LOOK = { 50.0f, 12, 10 };

    // 敵が現れるとき (PlaySpawn) 煙を出す高さと数 地面に広げる輪の大きさと色 (0xRRGGBB) と長さと濃さ
    struct SpawnLook {
        float smokeHeight;
        int smokeCount;
        float ringStartRadius;
        float ringEndRadius;
        float ringWidth;
        unsigned int ringColor;
        float ringLife;
        float ringAlpha;
    };
    const SpawnLook SPAWN_LOOK = { 80.0f, 12, 20.0f, 220.0f, 80.0f, 0x965AFF, 0.6f, 0.8f };

    // 回復の光の数 (PlayHeal)
    constexpr int HEAL_COUNT = 40;

    // 土煙 (PlayDust) 出す高さと散らす角度 湯気 (PlaySteam) を散らす角度
    constexpr float DUST_LIFT = 15.0f;
    constexpr float DUST_SPREAD = 75.0f;
    constexpr float STEAM_SPREAD = 30.0f;

    // 衝撃波 (PlayShockwave) 大きさと太さは渡された半径に掛ける割合
    // 外の輪 内側を速く走る白い芯 跳ね上がるかけら 土煙
    struct ShockwaveLook {
        float waveStartRate;
        float waveWidthRate;
        float waveLife;
        float coreStartRate;
        float coreEndRate;
        float coreWidthRate;
        unsigned int coreColor;
        float coreLife;
        float debrisHeight;
        float debrisSpread;
        int debrisCount;
        int dustCount;
    };
    const ShockwaveLook SHOCKWAVE_LOOK = {
        0.15f, 0.35f, 0.45f,            // waveStartRate waveWidthRate waveLife
        0.1f, 0.75f, 0.15f, 0xFFFFFF, 0.3f, // coreStartRate coreEndRate coreWidthRate coreColor coreLife
        10.0f, 55.0f, 14,               // debrisHeight debrisSpread debrisCount
        10,                             // dustCount
    };

    // フェーズの最後の 1 体を倒したとき (PlayFinalBlow) 普通のとどめとははっきり違う重さにする
    struct FinalBlowLook {
        // 一度ほぼ止める秒数 そのあとゆっくり流す速さ (1 で普通) と長さ 寄るのもこの長さ
        float hitStop;
        float slowScale;
        float slowTime;

        // 画面を光らせる色 (0xRRGGBB) と濃さと長さ
        unsigned int flashColor;
        float flashAlpha;
        float flashTime;

        // 寄る角度 度
        float zoom;

        // 画面の揺れの強さと長さ
        float shake;
        float shakeTime;

        // 金の衝撃波を二重に 内側は速く、外側は大きく 半径と色 (0xRRGGBB)
        float innerWaveRadius;
        unsigned int innerWaveColor;
        float outerWaveRadius;
        unsigned int outerWaveColor;

        // 粒を出す高さ (足元から) と、弾ける光 残り火 線の火花の数
        float burstHeight;
        int burstCount;
        int emberCount;
        int streakCount;
    };
    const FinalBlowLook FINAL_BLOW_LOOK = {
        0.2f,                       // hitStop
        0.15f, 1.3f,                // slowScale slowTime
        0xFFFFFF, 0.7f, 0.45f,      // flashColor flashAlpha flashTime
        14.0f,                      // zoom
        20.0f, 0.8f,                // shake shakeTime
        600.0f, 0xFFDC96,           // innerWaveRadius innerWaveColor
        1100.0f, 0xFFB45A,          // outerWaveRadius outerWaveColor
        100.0f, 40, 30, 30,         // burstHeight burstCount emberCount streakCount
    };

    // 吹き飛んだ体が壁にぶつかったとき (PlayWallHit)
    // 数と強さは いちばん弱いときの値 + ぶつかった強さ (0〜1) に掛けて足す値
    struct WallHitLook {
        float streakSpread;
        int streakCount;
        float streakExtra;
        float sparkSpread;
        int sparkCount;
        float sparkExtra;
        int dustCount;
        float dustExtra;
        float hitStop;
        float hitStopExtra;
        float shake;
        float shakeExtra;
        float shakeTime;

        // ぶつかった所の光の幕の強さ 0〜1
        float glow;
        float glowExtra;
    };
    const WallHitLook WALL_HIT_LOOK = {
        40.0f, 6, 10.0f,            // streakSpread streakCount streakExtra
        70.0f, 4, 6.0f,             // sparkSpread sparkCount sparkExtra
        6, 6.0f,                    // dustCount dustExtra
        0.03f, 0.04f,               // hitStop hitStopExtra
        3.0f, 7.0f, 0.25f,          // shake shakeExtra shakeTime
        0.5f, 0.5f,                 // glow glowExtra
    };

    // 壁が割れたとき (PlayWallBreak) かけらと線の火花 当たった点の光の数
    struct WallBreakLook {
        float shardSpread;
        int shardCount;
        float streakSpread;
        int streakCount;
        int flashCount;
    };
    const WallBreakLook WALL_BREAK_LOOK = { 45.0f, 36, 30.0f, 20, 2 };
}

EffectManager::~EffectManager() {
    if (_instance == this) _instance = nullptr;

    // ヒットストップやスローの途中でシーンが変わっても、次のシーンが止まったままにならないように
    Time::SetTimeScale(1.0f);

    if (_damageGraph != -1) DeleteGraph(_damageGraph);
    if (_deadGraph != -1) DeleteGraph(_deadGraph);
    if (_lightningGraph != -1) DeleteGraph(_lightningGraph);
    if (_wallCrackGraph != -1) DeleteGraph(_wallCrackGraph);
}

void EffectManager::Initialize(CameraFollow* camera) {
    _instance = this;
    _camera = camera;

    _damageGraph = LoadGraph("Data/Effect/Damage.png");
    _deadGraph = LoadGraph("Data/Effect/Dead.png");
    _lightningGraph = LoadGraph("Data/Effect/Lightning.png");
    _wallCrackGraph = LoadGraph("Data/Effect/WallCrack.png");

    // 斬った瞬間に散る火花
    _hitSpark = CreateSystem("HitSpark", _damageGraph);
    if (_hitSpark) {
        _hitSpark->startSizeMin = 25.0f;
        _hitSpark->startSizeMax = 60.0f;
        _hitSpark->startSpeedMin = 250.0f;
        _hitSpark->startSpeedMax = 800.0f;
        _hitSpark->startLifetimeMin = 0.12f;
        _hitSpark->startLifetimeMax = 0.3f;
        _hitSpark->drag = 2500.0f;
        _hitSpark->gravity = VGet(0.0f, -600.0f, 0.0f);
        _hitSpark->color = GetColorU8(255, 210, 150, 255);
    }

    // 斬った向きへ線を引いて飛ぶ火花 丸い粒だけより鋭く見える
    _hitStreak = CreateSystem("HitStreak", _damageGraph);
    if (_hitStreak) {
        _hitStreak->startSizeMin = 10.0f;
        _hitStreak->startSizeMax = 18.0f;
        _hitStreak->startSpeedMin = 900.0f;
        _hitStreak->startSpeedMax = 1700.0f;
        _hitStreak->startLifetimeMin = 0.1f;
        _hitStreak->startLifetimeMax = 0.2f;
        _hitStreak->drag = 4000.0f;
        _hitStreak->gravity = VGet(0.0f, -900.0f, 0.0f);
        _hitStreak->stretch = 0.035f;
        _hitStreak->color = GetColorU8(255, 235, 190, 255);
    }

    // 当たった点に一瞬だけ出る大きな光
    _hitFlash = CreateSystem("HitFlash", _damageGraph);
    if (_hitFlash) {
        _hitFlash->startSizeMin = 140.0f;
        _hitFlash->startSizeMax = 200.0f;
        _hitFlash->startSpeedMin = 0.0f;
        _hitFlash->startSpeedMax = 0.0f;
        _hitFlash->startLifetimeMin = 0.06f;
        _hitFlash->startLifetimeMax = 0.09f;
    }

    _guardSpark = CreateSystem("GuardSpark", _damageGraph);
    if (_guardSpark) {
        _guardSpark->startSizeMin = 12.0f;
        _guardSpark->startSizeMax = 24.0f;
        _guardSpark->startSpeedMin = 500.0f;
        _guardSpark->startSpeedMax = 1100.0f;
        _guardSpark->startLifetimeMin = 0.1f;
        _guardSpark->startLifetimeMax = 0.25f;
        _guardSpark->drag = 2500.0f;
        _guardSpark->gravity = VGet(0.0f, -1200.0f, 0.0f);
        _guardSpark->stretch = 0.03f;
        _guardSpark->color = GetColorU8(255, 230, 100, 255);
    }

    // フェーズの最後の 1 体を倒したとき (PlayFinalBlow) に弾ける光 厄災の魔物が消えるときの紫
    _killBurst = CreateSystem("KillBurst", _damageGraph);
    if (_killBurst) {
        _killBurst->emissionShape = EmissionShape::Sphere;
        _killBurst->emissionRadius = 30.0f;
        _killBurst->startSizeMin = 40.0f;
        _killBurst->startSizeMax = 90.0f;
        _killBurst->startSpeedMin = 300.0f;
        _killBurst->startSpeedMax = 900.0f;
        _killBurst->startLifetimeMin = 0.2f;
        _killBurst->startLifetimeMax = 0.45f;
        _killBurst->drag = 1800.0f;
        _killBurst->color = GetColorU8(200, 120, 255, 255);
    }

    // 同じく最後の 1 体を倒したあと、しばらく立ちのぼる残り火
    _killEmber = CreateSystem("KillEmber", _damageGraph);
    if (_killEmber) {
        _killEmber->emissionShape = EmissionShape::Sphere;
        _killEmber->emissionRadius = 50.0f;
        _killEmber->startSizeMin = 12.0f;
        _killEmber->startSizeMax = 26.0f;
        _killEmber->startSpeedMin = 60.0f;
        _killEmber->startSpeedMax = 200.0f;
        _killEmber->startLifetimeMin = 0.6f;
        _killEmber->startLifetimeMax = 1.1f;
        _killEmber->drag = 100.0f;
        _killEmber->gravity = VGet(0.0f, 220.0f, 0.0f);
        _killEmber->alphaFadeRatio = 0.5f;
        _killEmber->color = GetColorU8(255, 90, 180, 255);
    }

    // 敵が現れる場所の目印 急に湧いたように見えないように
    _spawnSmoke = CreateSystem("SpawnSmoke", _deadGraph);
    if (_spawnSmoke) {
        _spawnSmoke->emissionShape = EmissionShape::Sphere;
        _spawnSmoke->emissionRadius = 70.0f;
        _spawnSmoke->startSizeMin = 100.0f;
        _spawnSmoke->startSizeMax = 200.0f;
        _spawnSmoke->startSpeedMin = 30.0f;
        _spawnSmoke->startSpeedMax = 150.0f;
        _spawnSmoke->startLifetimeMin = 0.5f;
        _spawnSmoke->startLifetimeMax = 0.9f;
        _spawnSmoke->gravity = VGet(0.0f, 150.0f, 0.0f);
        _spawnSmoke->color = GetColorU8(130, 100, 210, 255);
    }

    _healLight = CreateSystem("HealLight", _damageGraph);
    if (_healLight) {
        _healLight->emissionShape = EmissionShape::Sphere;
        _healLight->emissionRadius = 90.0f;
        _healLight->startSizeMin = 30.0f;
        _healLight->startSizeMax = 70.0f;
        _healLight->startSpeedMin = 100.0f;
        _healLight->startSpeedMax = 300.0f;
        _healLight->startLifetimeMin = 0.6f;
        _healLight->startLifetimeMax = 1.1f;
        _healLight->gravity = VGet(0.0f, 400.0f, 0.0f);
        _healLight->color = GetColorU8(120, 255, 160, 255);
    }

    // 足元の土煙 光らせないので加算ではなく普通に重ねる
    _dust = CreateSystem("Dust", _deadGraph);
    if (_dust) {
        _dust->blendMode = DX_BLENDMODE_ALPHA;
        _dust->emissionShape = EmissionShape::Sphere;
        _dust->emissionRadius = 30.0f;
        _dust->startSizeMin = 60.0f;
        _dust->startSizeMax = 120.0f;
        _dust->startSpeedMin = 80.0f;
        _dust->startSpeedMax = 260.0f;
        _dust->startLifetimeMin = 0.35f;
        _dust->startLifetimeMax = 0.7f;
        _dust->drag = 400.0f;
        _dust->gravity = VGet(0.0f, 40.0f, 0.0f);
        _dust->color = GetColorU8(170, 150, 120, 255);
    }

    // 敵の攻撃が当たる直前の光 光った瞬間に回避すればジャスト回避になる
    _warningGlint = CreateSystem("WarningGlint", _damageGraph);
    if (_warningGlint) {
        _warningGlint->startSizeMin = 100.0f;
        _warningGlint->startSizeMax = 120.0f;
        _warningGlint->startSpeedMin = 0.0f;
        _warningGlint->startSpeedMax = 0.0f;
        _warningGlint->startLifetimeMin = 0.25f;
        _warningGlint->startLifetimeMax = 0.3f;
        _warningGlint->alphaFadeRatio = 0.5f;
        _warningGlint->color = GetColorU8(255, 170, 60, 255);
    }

    // ガードできない重い技の合図 普通の攻撃と見分けがつくよう赤く大きく
    _heavyGlint = CreateSystem("HeavyGlint", _damageGraph);
    if (_heavyGlint) {
        _heavyGlint->startSizeMin = 220.0f;
        _heavyGlint->startSizeMax = 260.0f;
        _heavyGlint->startSpeedMin = 0.0f;
        _heavyGlint->startSpeedMax = 0.0f;
        _heavyGlint->startLifetimeMin = 0.45f;
        _heavyGlint->startLifetimeMax = 0.5f;
        _heavyGlint->alphaFadeRatio = 0.5f;
        _heavyGlint->color = GetColorU8(255, 50, 40, 255);
    }

    // 衝撃波で跳ね上がる地面のかけら
    _shockDebris = CreateSystem("ShockDebris", _damageGraph);
    if (_shockDebris) {
        _shockDebris->emissionShape = EmissionShape::Sphere;
        _shockDebris->emissionRadius = 60.0f;
        _shockDebris->startSizeMin = 10.0f;
        _shockDebris->startSizeMax = 20.0f;
        _shockDebris->startSpeedMin = 400.0f;
        _shockDebris->startSpeedMax = 800.0f;
        _shockDebris->startLifetimeMin = 0.3f;
        _shockDebris->startLifetimeMax = 0.6f;
        _shockDebris->drag = 200.0f;
        _shockDebris->gravity = VGet(0.0f, -1500.0f, 0.0f);
        _shockDebris->stretch = 0.02f;
        _shockDebris->color = GetColorU8(230, 200, 160, 255);
    }

    // 吹っ飛ばされ値が溜まった体から立ちのぼる湯気 光らせず白く重ねる
    // 群れの全員から出ることがあるので、粒の数は多めに取っておく
    _steam = CreateSystem("Steam", _deadGraph);
    if (_steam) {
        _steam->maxParticles = 400;
        _steam->blendMode = DX_BLENDMODE_ALPHA;
        _steam->emissionShape = EmissionShape::Sphere;
        _steam->emissionRadius = 25.0f;
        _steam->startSizeMin = 40.0f;
        _steam->startSizeMax = 80.0f;
        _steam->startSpeedMin = 40.0f;
        _steam->startSpeedMax = 110.0f;
        _steam->startLifetimeMin = 0.6f;
        _steam->startLifetimeMax = 1.1f;
        _steam->drag = 40.0f;
        _steam->gravity = VGet(0.0f, 120.0f, 0.0f);
        _steam->alphaFadeRatio = 0.6f;
        _steam->color = GetColorU8(235, 235, 240, 255);
    }

    // 割れた壁のかけら 光の幕と同じ水色にして、壁が砕けたと分かるようにする
    _wallShard = CreateSystem("WallShard", _damageGraph);
    if (_wallShard) {
        _wallShard->emissionShape = EmissionShape::Sphere;
        _wallShard->emissionRadius = 40.0f;
        _wallShard->startSizeMin = 14.0f;
        _wallShard->startSizeMax = 30.0f;
        _wallShard->startSpeedMin = 600.0f;
        _wallShard->startSpeedMax = 1400.0f;
        _wallShard->startLifetimeMin = 0.4f;
        _wallShard->startLifetimeMax = 0.8f;
        _wallShard->drag = 500.0f;
        _wallShard->gravity = VGet(0.0f, -900.0f, 0.0f);
        _wallShard->stretch = 0.03f;
        _wallShard->color = GetColorU8(150, 220, 255, 255);
    }

    // 弧と輪 画像は剣の軌跡と同じものを使い、見た目をそろえる
    _shapes = gameObject->AddChild("ShapeEffects")->AddComponent<ShapeEffectRenderer>();
    _shapes->Setup("Data/Effect/SlashLocus.png", "Data/Effect/SphereLocus.png");

    // 雷と壁のヒビ 画像を 1 枚の板に貼って出す
    _sprites = gameObject->AddChild("SpriteEffects")->AddComponent<SpriteEffectRenderer>();
    _sprites->Setup();

    // 敵の足元の影と輪 地面に溶けて見えにくい敵を目立たせる
    _markers = gameObject->AddChild("Markers")->AddComponent<MarkerRenderer>();
    _markers->Setup();

    _screenFlash = gameObject->AddChild("ScreenFlash")->AddComponent<ScreenFlash>();
}

void EffectManager::Update(float deltaTime) {
    // 自分が止めた時間の中では数えられないので実時間で数える
    float unscaled = Time::UnscaledDeltaTime();

    if (_hitStopTimer > 0.0f) {
        _hitStopTimer -= unscaled;
        if (_hitStopTimer < 0.0f) _hitStopTimer = 0.0f;
    }
    if (_slowTimer > 0.0f) {
        _slowTimer -= unscaled;
        if (_slowTimer < 0.0f) _slowTimer = 0.0f;
    }

    ApplyTimeScale();
}

// ----- 粒 -----

void EffectManager::PlayHit(VECTOR position, VECTOR direction) {
    direction.y = 0.0f;
    if (VSquareSize(direction) > 0.0001f) {
        VECTOR aim = VAdd(VNorm(direction), VScale(UP, SPARK_LIFT));
        BurstToward(_hitStreak, position, aim, HIT_LOOK.streakSpread, HIT_LOOK.streakCount);
        BurstToward(_hitSpark, position, aim, HIT_LOOK.sparkSpread, HIT_LOOK.sparkCount);
    }
    else {
        Burst(_hitStreak, position, HIT_LOOK.aroundStreakCount);
        Burst(_hitSpark, position, HIT_LOOK.aroundSparkCount);
    }
    Burst(_hitFlash, position, 1);
}

void EffectManager::PlayGuard(VECTOR position, VECTOR direction) {
    direction.y = 0.0f;
    if (VSquareSize(direction) > 0.0001f) {
        VECTOR aim = VAdd(VNorm(direction), VScale(UP, SPARK_LIFT));
        BurstToward(_guardSpark, position, aim, GUARD_LOOK.spread, GUARD_LOOK.count);
    }
    else {
        Burst(_guardSpark, position, GUARD_LOOK.aroundCount);
    }
}

void EffectManager::PlaySpawn(VECTOR groundPosition) {
    const SpawnLook& look = SPAWN_LOOK;
    Burst(_spawnSmoke, VAdd(groundPosition, VGet(0.0f, look.smokeHeight, 0.0f)), look.smokeCount);

    // 湧き出る穴のように、紫の輪を地面に広げる
    if (_shapes) {
        ShapeEffectRenderer::RingDesc ring;
        ring.center = groundPosition;
        ring.startRadius = look.ringStartRadius;
        ring.endRadius = look.ringEndRadius;
        ring.width = look.ringWidth;
        ring.color = ToColor(look.ringColor);
        ring.life = look.ringLife;
        ring.alphaStart = look.ringAlpha;
        _shapes->AddRing(ring);
    }
}

void EffectManager::PlayHeal(VECTOR position) {
    Burst(_healLight, position, HEAL_COUNT);
}

void EffectManager::PlayDust(VECTOR groundPosition, int count) {
    // 地面に半分埋もれないよう、少し上から横と上へ広げる
    VECTOR position = VAdd(groundPosition, VGet(0.0f, DUST_LIFT, 0.0f));
    BurstToward(_dust, position, UP, DUST_SPREAD, count);
}

void EffectManager::PlayWarning(VECTOR position, bool isHeavy) {
    Burst(isHeavy ? _heavyGlint : _warningGlint, position, 1);
}

void EffectManager::PlaySteam(VECTOR position, int count) {
    BurstToward(_steam, position, UP, STEAM_SPREAD, count);
}

// ----- 形 -----

void EffectManager::PlaySlashArc(const ArcDesc& desc) {
    if (_shapes) _shapes->AddArc(desc);
}

void EffectManager::PlayShockwave(VECTOR groundPosition, float radius, COLOR_U8 color) {
    if (radius <= 0.0f) return;

    const ShockwaveLook& look = SHOCKWAVE_LOOK;
    if (_shapes) {
        ShapeEffectRenderer::RingDesc wave;
        wave.center = groundPosition;
        wave.startRadius = radius * look.waveStartRate;
        wave.endRadius = radius;
        wave.width = radius * look.waveWidthRate;
        wave.color = color;
        wave.life = look.waveLife;
        _shapes->AddRing(wave);

        // 内側を白く速く走らせ、衝撃の芯に見せる
        ShapeEffectRenderer::RingDesc core;
        core.center = groundPosition;
        core.startRadius = radius * look.coreStartRate;
        core.endRadius = radius * look.coreEndRate;
        core.width = radius * look.coreWidthRate;
        core.color = ToColor(look.coreColor);
        core.life = look.coreLife;
        _shapes->AddRing(core);
    }

    VECTOR debrisPosition = VAdd(groundPosition, VGet(0.0f, look.debrisHeight, 0.0f));
    BurstToward(_shockDebris, debrisPosition, UP, look.debrisSpread, look.debrisCount);
    PlayDust(groundPosition, look.dustCount);
}

// ----- 画像の板 -----

void EffectManager::PlayLightning(SpriteDesc desc) {
    if (!_sprites) return;
    desc.graph = _lightningGraph;
    _sprites->Add(desc);
}

void EffectManager::PlayWallCrack(SpriteDesc desc) {
    if (!_sprites) return;
    desc.graph = _wallCrackGraph;
    _sprites->Add(desc);
}

// ----- 画面 -----

void EffectManager::FlashScreen(unsigned int color, float alpha, float seconds) {
    if (_screenFlash) _screenFlash->Flash(color, alpha, seconds);
}

void EffectManager::ZoomPunch(float degrees, float seconds) {
    if (_camera) _camera->ZoomPunch(degrees, seconds);
}

void EffectManager::HitStop(float seconds) {
    if (seconds <= 0.0f) return;
    if (seconds > _hitStopTimer) _hitStopTimer = seconds;
    ApplyTimeScale();
}

void EffectManager::Shake(float power, float seconds) {
    if (_camera) _camera->Shake(power, seconds);
}

void EffectManager::SlowMotion(float scale, float seconds) {
    if (seconds <= 0.0f) return;

    // 重なったら、より遅く、より長いほうを残す
    if (_slowTimer <= 0.0f || scale < _slowScale) _slowScale = scale;
    if (seconds > _slowTimer) _slowTimer = seconds;
    ApplyTimeScale();
}

void EffectManager::PlayFinalBlow(VECTOR position) {
    const FinalBlowLook& look = FINAL_BLOW_LOOK;

    // 一度ほぼ止めてから、ゆっくり流す
    HitStop(look.hitStop);
    SlowMotion(look.slowScale, look.slowTime);

    FlashScreen(look.flashColor, look.flashAlpha, look.flashTime);
    ZoomPunch(look.zoom, look.slowTime);

    // 大きく長く揺らす
    Shake(look.shake, look.shakeTime);

    PlayShockwave(position, look.innerWaveRadius, ToColor(look.innerWaveColor));
    PlayShockwave(position, look.outerWaveRadius, ToColor(look.outerWaveColor));

    VECTOR center = VAdd(position, VGet(0.0f, look.burstHeight, 0.0f));
    Burst(_killBurst, center, look.burstCount);
    Burst(_killEmber, center, look.emberCount);
    Burst(_hitStreak, center, look.streakCount);
}

void EffectManager::PlayWallHit(VECTOR position, VECTOR normal, float power) {
    const WallHitLook& look = WALL_HIT_LOOK;

    // 壁から内側、跳ね返る向きへ火花を散らす
    VECTOR aim = VAdd(normal, VScale(UP, SPARK_LIFT));
    BurstToward(_hitStreak, position, aim, look.streakSpread, look.streakCount + static_cast<int>(power * look.streakExtra));
    BurstToward(_hitSpark, position, aim, look.sparkSpread, look.sparkCount + static_cast<int>(power * look.sparkExtra));
    Burst(_hitFlash, position, 1);

    PlayDust(StageBuilder::SnapToGround(position), look.dustCount + static_cast<int>(power * look.dustExtra));

    HitStop(look.hitStop + power * look.hitStopExtra);
    Shake(look.shake + power * look.shakeExtra, look.shakeTime);

    // 壁は離れていると見えないので、ぶつかった所だけ光らせて、そこに壁があると分かるようにする
    if (auto* boundary = ArenaBoundary::Get()) boundary->Flash(position, look.glow + power * look.glowExtra);
}

void EffectManager::PlayWallBreak(VECTOR position, VECTOR outward) {
    outward.y = 0.0f;
    if (VSquareSize(outward) < 0.0001f) outward = VGet(0.0f, 0.0f, 1.0f);

    // かけらと火花は外へ、少し上向きに散らす
    const WallBreakLook& look = WALL_BREAK_LOOK;
    VECTOR aim = VAdd(VNorm(outward), VScale(UP, SHARD_LIFT));
    BurstToward(_wallShard, position, aim, look.shardSpread, look.shardCount);
    BurstToward(_hitStreak, position, aim, look.streakSpread, look.streakCount);
    Burst(_hitFlash, position, look.flashCount);
}

void EffectManager::SetBaseTimeScale(float scale) {
    _baseTimeScale = scale;
    ApplyTimeScale();
}

// ----- 内部 -----

ParticleSystem* EffectManager::CreateSystem(const char* name, int graph) {
    auto* child = gameObject->AddChild(name);
    auto* system = child->AddComponent<ParticleSystem>();

    system->graphHandle = graph;
    system->blendMode = DX_BLENDMODE_ADD;
    system->looping = false;
    // 自分で Emit したときだけ出したいので、放出の時間は極く短くしておく
    system->duration = 0.01f;
    system->maxParticles = 256;
    return system;
}

void EffectManager::Burst(ParticleSystem* system, VECTOR position, int count) {
    if (!system) return;
    system->spreadDegree = 180.0f;
    system->transform->localPosition = position;
    system->Emit(count);
}

void EffectManager::BurstToward(ParticleSystem* system, VECTOR position, VECTOR direction, float spreadDegree, int count) {
    if (!system) return;
    system->emitDirection = direction;
    system->spreadDegree = spreadDegree;
    system->transform->localPosition = position;
    system->Emit(count);
}

void EffectManager::ApplyTimeScale() {
    // 止める理由が重なったら掛け合わせる スロー中の一撃はさらに止まる
    float scale = _baseTimeScale;
    if (_hitStopTimer > 0.0f) scale *= HIT_STOP_TIME_SCALE;
    if (_slowTimer > 0.0f) scale *= _slowScale;
    Time::SetTimeScale(scale);
}
