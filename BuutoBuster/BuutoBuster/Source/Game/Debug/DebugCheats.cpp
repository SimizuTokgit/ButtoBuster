#include "DebugCheats.h"
#include "DebugMenu.h"
#include "Player.h"
#include "PhaseDirector.h"
#include "EffectManager.h"
#include "Hud.h"

namespace {
    const char* const INVINCIBLE = "無敵";
    const char* const SLOW = "スロー";
    const char* const SHOW_STATE = "状態を表示";

    // スローの速さ 動きを1コマずつ確かめられるくらい
    constexpr float SLOW_TIME_SCALE = 0.25f;
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
    menu.AddCommand(KEY_INPUT_F7, "Goblin を出す", [director]() {
        director->SpawnImmediately(EnemyKind::Goblin);
    });
    menu.AddCommand(KEY_INPUT_F8, "Bee を出す", [director]() {
        director->SpawnImmediately(EnemyKind::Bee);
    });
    menu.AddCommand(KEY_INPUT_F9, "Golem を出す", [director]() {
        director->SpawnImmediately(EnemyKind::Golem);
    });
}

void DebugCheats::Update(float deltaTime) {
    const auto& menu = DebugMenu::Instance();

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
