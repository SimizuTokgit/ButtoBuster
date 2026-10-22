#include "DebugCheats.h"
#include "DebugMenu.h"
#include "Player.h"
#include "PhaseDirector.h"
#include "EffectManager.h"
#include "Hud.h"
#include "StatePool.h"
#include "CameraFollow.h"
#include "MarkerRenderer.h"
#include "Enemy.h"
#include "EnemyAI.h"
#include "StageBuilder.h"
#include "Difficulty.h"
#include "Transform.h"
#include <cmath>
#include <cstdio>
#include <string>

namespace {
    const char* const INVINCIBLE = "無敵";
    const char* const SLOW = "スロー";
    const char* const SHOW_STATE = "状態を表示";
    const char* const FLAT_MARKERS = "撮影: 足元の印と攻撃範囲を地面に沿わせない";
    const char* const OVERHEAD = "撮影: 真上近くから見下ろす";

    // スローの速さ 動きを1コマずつ確かめられるくらい
    constexpr float SLOW_TIME_SCALE = 0.25f;

    // F6 で一度に進めるフェーズの数
    constexpr int SKIP_MANY_PHASES = 5;

    // F12 で一度に出す敵の数 たくさん出して、状態のメモリと作り直しの回数を確かめる
    constexpr int SPAWN_MANY_COUNT = 10;

    // 撮影 賢さ 1〜5 の動きの違いを、同じ状況から撮って比べる
    // カメラの向いている先に、扇に並べて出す 出たところから、賢さによって回り込み方が変わるのが見えるように
    struct FilmingSetup {
        int count;              // 出すグレゴブリンの数
        float distance;         // プレイヤーからの距離
        float spreadDegree;     // 両端の敵の間の角度
        int seed;               // くじの始まり 毎回同じにして、回る向きや大振りの引きを同じにする
    };
    constexpr FilmingSetup FILMING = { 4, 700.0f, 60.0f, 2027 };
}

void DebugCheats::Setup(Player* player, PhaseDirector* director, Hud* hud, CameraFollow* camera) {
    _player = player;
    _director = director;
    _hud = hud;
    _camera = camera;

    auto& menu = DebugMenu::Instance();

    // 切り替えはシーンをまたいで残る 一度付けた無敵が次のゲームでも効くように
    menu.AddToggle(KEY_INPUT_F3, INVINCIBLE);
    menu.AddToggle(KEY_INPUT_F10, SLOW);
    menu.AddToggle(KEY_INPUT_F11, SHOW_STATE);
    menu.AddToggle(KEY_INPUT_4, FLAT_MARKERS);
    menu.AddToggle(KEY_INPUT_5, OVERHEAD);

    // コマンドはシーンを変えると消える このシーンのものを掴んでいるため
    menu.AddCommand(KEY_INPUT_F4, "敵を全滅", [director]() {
        director->DefeatAllEnemies();
    });
    menu.AddCommand(KEY_INPUT_F5, "次のフェーズへ", [director]() {
        director->SkipPhases(1);
    });
    menu.AddCommand(KEY_INPUT_F6, std::to_string(SKIP_MANY_PHASES) + "フェーズ進める", [director]() {
        director->SkipPhases(SKIP_MANY_PHASES);
    });
    menu.AddCommand(KEY_INPUT_F7, "グレゴブリン を出す", [director]() {
        director->SpawnImmediately(EnemyKind::Goblin);
    });
    menu.AddCommand(KEY_INPUT_F8, "ビーザトール を出す", [director]() {
        director->SpawnImmediately(EnemyKind::Bee);
    });
    menu.AddCommand(KEY_INPUT_F9, "ロックマキナ を出す", [director]() {
        director->SpawnImmediately(EnemyKind::Golem);
    });
    menu.AddCommand(KEY_INPUT_F12, "グレゴブリン を " + std::to_string(SPAWN_MANY_COUNT) + " 体出す", [director]() {
        for (int i = 0; i < SPAWN_MANY_COUNT; ++i) director->SpawnImmediately(EnemyKind::Goblin);
    });

    // F キーはすべて埋まっているので、数字の 1 にする 必殺技の演出を何度も確かめるため
    menu.AddCommand(KEY_INPUT_1, "必殺技のゲージを満タンにする", [player]() {
        player->FillSpecialGauge();
    });

    // 撮影 押すとフェーズが止まり、出した敵だけになる 普段の遊びに戻すときは、タイトルからやり直す
    menu.AddCommand(KEY_INPUT_2, "撮影: 賢さを 1→5 の順に変えて始め直す", [this]() {
        _filmingIntelligence = (_filmingIntelligence % GameMode::MAX_INTELLIGENCE) + 1;
        StartFilming();
    });
    menu.AddCommand(KEY_INPUT_3, "撮影: 同じ賢さで始め直す", [this]() {
        if (_filmingIntelligence < GameMode::MIN_INTELLIGENCE) _filmingIntelligence = GameMode::MIN_INTELLIGENCE;
        StartFilming();
    });

    // 状態の作り直しと、そのメモリ (StatePool)
    // 今までの作りなら、作り直すたびにヒープから取っていた 置き場で使い回すと、ヒープから取るのは初めのうちだけになる
    menu.AddInfo([director, this]() {
        char text[160];
        snprintf(text, sizeof(text), "敵 %d 体   状態の作り直し 毎秒 %lld 回 (合計 %lld 回)",
            static_cast<int>(director->GetEnemies().size()), _createdPerSecond, StatePool::GetStats().createdCount);
        return std::string(text);
    });
    menu.AddInfo([this]() {
        const auto& stats = StatePool::GetStats();
        char text[160];
        snprintf(text, sizeof(text), "うちヒープから取った 毎秒 %lld 回 (合計 %lld 回)   置き場 %.1f KB / 使用中 %.1f KB (%d 個)",
            _heapPerSecond, stats.heapCount, stats.heapBytes / 1024.0, stats.usedBytes / 1024.0, stats.aliveCount);
        return std::string(text);
    });
}

void DebugCheats::StartFilming() {
    SRand(FILMING.seed);
    _director->StopPhases();

    _player->ReturnToSpawn();
    _player->ResetBlow();
    if (_hud) _hud->filmingIntelligence = _filmingIntelligence;

    // 表の数値のまま、賢さだけを変える 難易度の足し引きもしない
    _filmingGoblin = EnemyDatabase::Get(EnemyKind::Goblin);
    _filmingGoblin.intelligence = _filmingIntelligence;

    VECTOR center = _player->GetPosition();
    VECTOR forward = _camera ? _camera->GetGroundForward() : _player->GetForward();
    float middle = atan2f(forward.z, forward.x);
    float spread = Transform::Deg2Rad(FILMING.spreadDegree);

    for (int i = 0; i < FILMING.count; ++i) {
        float rate = (FILMING.count > 1) ? static_cast<float>(i) / (FILMING.count - 1) - 0.5f : 0.0f;
        float angle = middle + spread * rate;
        VECTOR position = VAdd(center, VGet(cosf(angle) * FILMING.distance, 0.0f, sinf(angle) * FILMING.distance));

        float groundY = center.y;
        StageBuilder::FindGroundHeight(position.x, position.z, groundY);
        position.y = groundY + _director->GetSpawnHeight(_filmingGoblin);

        Enemy* enemy = _director->SpawnAt(_filmingGoblin, position);
        if (!enemy) continue;
        if (auto* ai = enemy->GetComponent<EnemyAI>()) ai->followsDifficulty = false;
    }
}

void DebugCheats::Update(float deltaTime) {
    const auto& menu = DebugMenu::Instance();

    // 1 秒ごとに、状態を作り直した回数とヒープから取った回数を数える
    _poolTimer += Time::UnscaledDeltaTime();
    if (_poolTimer >= 1.0f) {
        const auto& stats = StatePool::GetStats();
        _createdPerSecond = stats.createdCount - _lastCreatedCount;
        _heapPerSecond = stats.heapCount - _lastHeapCount;
        _lastCreatedCount = stats.createdCount;
        _lastHeapCount = stats.heapCount;
        _poolTimer -= 1.0f;
    }

    if (_player) _player->isCheatInvincible = menu.GetToggle(INVINCIBLE);
    if (_hud) _hud->isStateVisible = menu.GetToggle(SHOW_STATE);
    MarkerRenderer::followsGround = !menu.GetToggle(FLAT_MARKERS);
    if (_camera) _camera->isOverhead = menu.GetToggle(OVERHEAD);

    // 変わったときだけ伝える 毎フレーム入れるとヒットストップと取り合いになる
    bool isSlow = menu.GetToggle(SLOW);
    if (isSlow != _wasSlow) {
        _wasSlow = isSlow;
        if (auto* effects = EffectManager::Get()) {
            effects->SetBaseTimeScale(isSlow ? SLOW_TIME_SCALE : 1.0f);
        }
    }
}
