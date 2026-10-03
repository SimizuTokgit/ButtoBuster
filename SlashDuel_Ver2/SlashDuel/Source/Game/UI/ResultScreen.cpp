#include "ResultScreen.h"
#include "GameFont.h"
#include "PhaseDirector.h"
#include "Player.h"
#include "InputSystem.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "Time.h"
#include "TitleScene.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

void ResultScreen::Setup(Player* player, PhaseDirector* director) {
    _player = player;
    _director = director;
    sortingOrder = 20;
}

void ResultScreen::Update(float deltaTime) {
    if (!_director || !_director->IsResultReady()) return;

    // ヒットストップの最中に倒れても止まらないよう実時間で数える
    _shownTime += Time::UnscaledDeltaTime();

    if (_isRequested || _shownTime < INPUT_DELAY) return;
    if (!InputSystem::Instance().ConfirmPressed()) return;

    _isRequested = true;
    SoundManager::Instance().PlaySE("Common/system_enter");
    SceneManager::Instance().RequestLoadScene<TitleScene>();
}

void ResultScreen::Render() {
    if (!_director || _director->GetStep() != PhaseDirector::Step::GameOver) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);
    int centerX = screenWidth / 2;

    // 倒れてから少しずつ暗くする
    float darkness = _director->GetStepTimer() / 2.0f;
    if (darkness > 1.0f) darkness = 1.0f;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(darkness * 190.0f));
    DrawBox(0, 0, screenWidth, screenHeight, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    if (!_director->IsResultReady()) return;

    int top = screenHeight / 2 - 190;
    char text[64];

    GameFont::DrawCentered(centerX, top, "GAME OVER", 0xFF6060, GameFont::Size::Huge);

    snprintf(text, sizeof(text), "到達フェーズ  %d", _director->GetPhase());
    GameFont::DrawCentered(centerX, top + 130, text, 0xFFFFFF, GameFont::Size::Large);

    snprintf(text, sizeof(text), "撃破  %d      最大コンボ  %d",
        _director->GetKillCount(), _player ? _player->GetMaxCombo() : 0);
    GameFont::DrawCentered(centerX, top + 206, text, 0xE0E0E0, GameFont::Size::Medium);

    snprintf(text, sizeof(text), "最高記録  フェーズ %d", _director->GetBestPhase());
    GameFont::DrawCentered(centerX, top + 252, text, 0xC0C0C0, GameFont::Size::Medium);

    if (_director->IsNewRecord()) {
        GameFont::DrawCentered(centerX, top + 298, "NEW RECORD", 0xFFD060, GameFont::Size::Medium);
    }

    // 点滅させる
    if (_shownTime >= INPUT_DELAY && fmodf(_shownTime, 1.0f) < 0.6f) {
        GameFont::DrawCentered(centerX, top + 370, "SPACE / A でタイトルへ", 0xAAAAAA, GameFont::Size::Small);
    }
}
