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
}

EffectManager::~EffectManager() {
    if (_instance == this) _instance = nullptr;

    // ヒットストップやスローの途中でシーンが変わっても、次のシーンが止まったままにならないように
    Time::SetTimeScale(1.0f);

    if (_damageGraph != -1) DeleteGraph(_damageGraph);
    if (_deadGraph != -1) DeleteGraph(_deadGraph);
}

void EffectManager::Initialize(CameraFollow* camera) {
    _instance = this;
    _camera = camera;

    _damageGraph = LoadGraph("Data/Effect/Damage.png");
    _deadGraph = LoadGraph("Data/Effect/Dead.png");

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

    // 倒した瞬間に弾ける光 厄災の魔物が消えるときの紫
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

    // 倒した後にしばらく立ちのぼる残り火
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

    // 倒した敵から立ち上る煙
    _deathSmoke = CreateSystem("DeathSmoke", _deadGraph);
    if (_deathSmoke) {
        _deathSmoke->emissionShape = EmissionShape::Sphere;
        _deathSmoke->emissionRadius = 40.0f;
        _deathSmoke->startSizeMin = 80.0f;
        _deathSmoke->startSizeMax = 160.0f;
        _deathSmoke->startSpeedMin = 60.0f;
        _deathSmoke->startSpeedMax = 260.0f;
        _deathSmoke->startLifetimeMin = 0.35f;
        _deathSmoke->startLifetimeMax = 0.7f;
        _deathSmoke->drag = 300.0f;
        _deathSmoke->gravity = VGet(0.0f, 250.0f, 0.0f);
        _deathSmoke->color = GetColorU8(220, 190, 255, 255);
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

    // 敵が振りかぶった瞬間の光 見ていれば避けられるようにする
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
        BurstToward(_hitStreak, position, aim, 35.0f, 9);
        BurstToward(_hitSpark, position, aim, 60.0f, 6);
    }
    else {
        Burst(_hitStreak, position, 8);
        Burst(_hitSpark, position, 8);
    }
    Burst(_hitFlash, position, 1);
}

void EffectManager::PlayGuard(VECTOR position, VECTOR direction) {
    direction.y = 0.0f;
    if (VSquareSize(direction) > 0.0001f) {
        VECTOR aim = VAdd(VNorm(direction), VScale(UP, SPARK_LIFT));
        BurstToward(_guardSpark, position, aim, 50.0f, 12);
    }
    else {
        Burst(_guardSpark, position, 10);
    }
}

void EffectManager::PlayKill(VECTOR position, VECTOR direction) {
    Burst(_killBurst, position, 16);
    Burst(_killEmber, position, 10);

    // 吹き飛ぶ向きへもう一度火花を散らし、最後の一撃を目立たせる
    direction.y = 0.0f;
    if (VSquareSize(direction) > 0.0001f) {
        VECTOR aim = VAdd(VNorm(direction), VScale(UP, SPARK_LIFT));
        BurstToward(_hitStreak, position, aim, 25.0f, 16);
    }
    Burst(_hitFlash, position, 2);

    // とどめの手応え 普通の当たりより長く止め、強く揺らし、少し寄る
    HitStop(0.09f);
    Shake(9.0f, 0.3f);
    ZoomPunch(4.0f, 0.35f);
    FlashScreen(0xFFFFFF, 0.18f, 0.12f);

    // 足元に紫の輪を広げる 倒した場所が群れの中でも分かるように
    VECTOR ground = position;
    StageBuilder::FindGroundHeight(position.x, position.z, ground.y);
    if (_shapes) {
        ShapeEffectRenderer::RingDesc ring;
        ring.center = ground;
        ring.startRadius = 30.0f;
        ring.endRadius = 260.0f;
        ring.width = 70.0f;
        ring.color = GetColorU8(200, 110, 255, 255);
        ring.life = 0.4f;
        _shapes->AddRing(ring);
    }
}

void EffectManager::PlayDeath(VECTOR position) {
    Burst(_deathSmoke, position, 14);
}

void EffectManager::PlaySpawn(VECTOR groundPosition) {
    Burst(_spawnSmoke, VAdd(groundPosition, VGet(0.0f, 80.0f, 0.0f)), 12);

    // 湧き出る穴のように、紫の輪を地面に広げる
    if (_shapes) {
        ShapeEffectRenderer::RingDesc ring;
        ring.center = groundPosition;
        ring.startRadius = 20.0f;
        ring.endRadius = 220.0f;
        ring.width = 80.0f;
        ring.color = GetColorU8(150, 90, 255, 255);
        ring.life = 0.6f;
        ring.alphaStart = 0.8f;
        _shapes->AddRing(ring);
    }
}

void EffectManager::PlayHeal(VECTOR position) {
    Burst(_healLight, position, 40);
}

void EffectManager::PlayDust(VECTOR groundPosition, int count) {
    // 地面に半分埋もれないよう、少し上から横と上へ広げる
    VECTOR position = VAdd(groundPosition, VGet(0.0f, 15.0f, 0.0f));
    BurstToward(_dust, position, UP, 75.0f, count);
}

void EffectManager::PlayWarning(VECTOR position, bool isHeavy) {
    Burst(isHeavy ? _heavyGlint : _warningGlint, position, 1);
}

void EffectManager::PlaySteam(VECTOR position, int count) {
    BurstToward(_steam, position, UP, 30.0f, count);
}

// ----- 形 -----

void EffectManager::PlaySlashArc(const ArcDesc& desc) {
    if (_shapes) _shapes->AddArc(desc);
}

void EffectManager::PlayShockwave(VECTOR groundPosition, float radius, COLOR_U8 color) {
    if (radius <= 0.0f) return;

    if (_shapes) {
        ShapeEffectRenderer::RingDesc wave;
        wave.center = groundPosition;
        wave.startRadius = radius * 0.15f;
        wave.endRadius = radius;
        wave.width = radius * 0.35f;
        wave.color = color;
        wave.life = 0.45f;
        _shapes->AddRing(wave);

        // 内側を白く速く走らせ、衝撃の芯に見せる
        ShapeEffectRenderer::RingDesc core;
        core.center = groundPosition;
        core.startRadius = radius * 0.1f;
        core.endRadius = radius * 0.75f;
        core.width = radius * 0.15f;
        core.color = GetColorU8(255, 255, 255, 255);
        core.life = 0.3f;
        _shapes->AddRing(core);
    }

    BurstToward(_shockDebris, VAdd(groundPosition, VGet(0.0f, 10.0f, 0.0f)), UP, 55.0f, 14);
    PlayDust(groundPosition, 10);
}

void EffectManager::PlayAreaWarning(VECTOR groundPosition, float radius, float seconds) {
    if (!_shapes || radius <= 0.0f || seconds <= 0.0f) return;

    // 中から外へ満ちていく赤 満ちきったときに当たる
    ShapeEffectRenderer::RingDesc fill;
    fill.center = groundPosition;
    fill.startRadius = 0.0f;
    fill.endRadius = radius;
    fill.width = radius;
    fill.color = GetColorU8(255, 40, 30, 255);
    fill.life = seconds;
    fill.alphaStart = 0.15f;
    fill.alphaEnd = 0.6f;
    _shapes->AddRing(fill);

    // 範囲の縁は最初から見せておく どこまで逃げればよいか分かるように
    ShapeEffectRenderer::RingDesc edge;
    edge.center = groundPosition;
    edge.startRadius = radius;
    edge.endRadius = radius;
    edge.width = 40.0f;
    edge.color = GetColorU8(255, 60, 40, 255);
    edge.life = seconds;
    edge.alphaStart = 0.5f;
    edge.alphaEnd = 0.9f;
    _shapes->AddRing(edge);
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
    // 一度ほぼ止めてから、ゆっくり流す
    HitStop(0.2f);
    SlowMotion(0.15f, 1.3f);

    FlashScreen(0xFFFFFF, 0.7f, 0.45f);
    ZoomPunch(14.0f, 1.3f);

    // 大きく長く揺らす 普通のとどめとははっきり違う重さにする
    Shake(20.0f, 0.8f);

    // 金の衝撃波を二重に 内側は速く、外側は大きく
    PlayShockwave(position, 600.0f, GetColorU8(255, 220, 150, 255));
    PlayShockwave(position, 1100.0f, GetColorU8(255, 180, 90, 255));

    VECTOR center = VAdd(position, VGet(0.0f, 100.0f, 0.0f));
    Burst(_killBurst, center, 40);
    Burst(_killEmber, center, 30);
    Burst(_hitStreak, center, 30);
}

void EffectManager::PlayWallHit(VECTOR position, VECTOR normal, float power) {
    // 壁から内側、跳ね返る向きへ火花を散らす
    VECTOR aim = VAdd(normal, VScale(UP, SPARK_LIFT));
    BurstToward(_hitStreak, position, aim, 40.0f, 6 + static_cast<int>(power * 10.0f));
    BurstToward(_hitSpark, position, aim, 70.0f, 4 + static_cast<int>(power * 6.0f));
    Burst(_hitFlash, position, 1);

    VECTOR ground = position;
    StageBuilder::FindGroundHeight(position.x, position.z, ground.y);
    PlayDust(ground, 6 + static_cast<int>(power * 6.0f));

    HitStop(0.03f + power * 0.04f);
    Shake(3.0f + power * 7.0f, 0.25f);

    // 壁は離れていると見えないので、ぶつかった所だけ光らせて、そこに壁があると分かるようにする
    if (auto* boundary = ArenaBoundary::Get()) boundary->Flash(position, 0.5f + power * 0.5f);
}

void EffectManager::PlayWallBreak(VECTOR position, VECTOR outward) {
    outward.y = 0.0f;
    if (VSquareSize(outward) < 0.0001f) outward = VGet(0.0f, 0.0f, 1.0f);

    // かけらと火花は外へ、少し上向きに散らす
    VECTOR aim = VAdd(VNorm(outward), VScale(UP, 0.4f));
    BurstToward(_wallShard, position, aim, 45.0f, 36);
    BurstToward(_hitStreak, position, aim, 30.0f, 20);
    Burst(_hitFlash, position, 2);
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
