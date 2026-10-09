#include "DebugCheats.h"
#include "DebugMenu.h"
#include "Player.h"
#include "PhaseDirector.h"
#include "EffectManager.h"
#include "Hud.h"
#include "StatePool.h"
#include <cstdio>
#include <string>

namespace {
    const char* const INVINCIBLE = "無敵";
    const char* const SLOW = "スロー";
    const char* const SHOW_STATE = "状態を表示";

    // スローの速さ 動きを1コマずつ確かめられるくらい
    constexpr float SLOW_TIME_SCALE = 0.25f;

    // F12 で一度に出す敵の数 たくさん出して、状態のメモリと作り直しの回数を確かめる
    constexpr int SPAWN_MANY_COUNT = 10;
}

void DebugCheats::Setup(Player* player, PhaseDirector* director, Hud* hud) {
    _player = player;
    _director = director;
    _hud = hud;

    auto& menu = DebugMenu::Instance();

    // 切り替えはシーンをまたいで残る 一度付けた無敵が次のゲームでも効くように
    menu.AddToggle(KEY_INPUT_F3, INVINCIBLE);
    menu.AddToggle(KEY_INPUT_F10, SLOW);
    menu.AddToggle(KEY_INPUT_F11, SHOW_STATE);

    // コマンドはシーンを変えると消える このシーンのものを掴んでいるため
    menu.AddCommand(KEY_INPUT_F4, "敵を全滅", [director]() {
        director->DefeatAllEnemies();
    });
    menu.AddCommand(KEY_INPUT_F5, "次のフェーズへ", [director]() {
        director->SkipPhases(1);
    });
    menu.AddCommand(KEY_INPUT_F6, "5フェーズ進める", [director]() {
        director->SkipPhases(5);
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
    menu.AddCommand(KEY_INPUT_F12, "グレゴブリン を 10 体出す", [director]() {
        for (int i = 0; i < SPAWN_MANY_COUNT; ++i) director->SpawnImmediately(EnemyKind::Goblin);
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

    // 変わったときだけ伝える 毎フレーム入れるとヒットストップと取り合いになる
    bool isSlow = menu.GetToggle(SLOW);
    if (isSlow != _wasSlow) {
        _wasSlow = isSlow;
        if (auto* effects = EffectManager::Get()) {
            effects->SetBaseTimeScale(isSlow ? SLOW_TIME_SCALE : 1.0f);
        }
    }
}
