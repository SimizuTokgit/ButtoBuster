#include "ResultScreen.h"
#include "GameFont.h"
#include "PhaseDirector.h"
#include "Player.h"
#include "InputSystem.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "Time.h"
#include "TitleScene.h"
#include "Difficulty.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

namespace {
    // ----- 結果の画面の見た目 -----

    // 画面を暗くしきるまでの秒数と、いちばん暗いときの濃さ 0〜255
    constexpr float DARKEN_TIME = 2.0f;
    constexpr int DARKNESS = 190;

    // 負けたときの大きな文字の色 (0xRRGGBB)
    constexpr unsigned int GAME_OVER_COLOR = 0xFF6060;

    // 勝ったときの大きな文字の色 (0xRRGGBB) と、浮かび上がるまでの秒数
    // 勝ったときは結果より先にこの文字だけを出す 結果が出るまでの間は PhaseData の victoryResultDelay
    constexpr unsigned int VICTORY_COLOR = 0xFFD060;
    constexpr float VICTORY_FADE_TIME = 0.4f;
}

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
    SoundManager::Instance().PlaySE("Game/SE_ButtonPush");
    SceneManager::Instance().RequestLoadScene<TitleScene>();
}

void ResultScreen::Render() {
    if (!_director) return;

    PhaseDirector::Step step = _director->GetStep();
    bool isVictory = step == PhaseDirector::Step::Victory;
    if (!isVictory && step != PhaseDirector::Step::GameOver) return;

    // 負けたときは倒れたときから、勝ったときは VICTORY を出すときから数える
    float time = _director->GetStepTimer();
    if (isVictory) time -= _director->data.victoryDelay;
    if (time < 0.0f) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);
    int centerX = screenWidth / 2;

    // 少しずつ暗くする
    float darkness = time / DARKEN_TIME;
    if (darkness > 1.0f) darkness = 1.0f;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(darkness * DARKNESS));
    DrawBox(0, 0, screenWidth, screenHeight, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    int top = screenHeight / 2 - 190;

    // 勝ったときは、結果より先に VICTORY だけを浮かび上がらせる
    if (isVictory) {
        float alpha = (VICTORY_FADE_TIME > 0.0f) ? time / VICTORY_FADE_TIME : 1.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
        GameFont::DrawCentered(centerX, top, "VICTORY", VICTORY_COLOR, GameFont::Size::Huge);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    if (!_director->IsResultReady()) return;

    char text[64];

    if (isVictory) {
        snprintf(text, sizeof(text), "フェーズ %d クリア", _director->GetPhase());
    }
    else {
        GameFont::DrawCentered(centerX, top, "GAME OVER", GAME_OVER_COLOR, GameFont::Size::Huge);
        snprintf(text, sizeof(text), "到達フェーズ  %d", _director->GetPhase());
    }
    GameFont::DrawCentered(centerX, top + 130, text, 0xFFFFFF, GameFont::Size::Large);

    snprintf(text, sizeof(text), "撃破  %d      最大コンボ  %d",
        _director->GetKillCount(), _player ? _player->GetMaxCombo() : 0);
    GameFont::DrawCentered(centerX, top + 206, text, 0xE0E0E0, GameFont::Size::Medium);

    snprintf(text, sizeof(text), "%s の最高記録  フェーズ %d", GameMode::GetData().name, _director->GetBestPhase());
    GameFont::DrawCentered(centerX, top + 252, text, 0xC0C0C0, GameFont::Size::Medium);

    if (_director->IsNewRecord()) {
        GameFont::DrawCentered(centerX, top + 298, "NEW RECORD", 0xFFD060, GameFont::Size::Medium);
    }

    // 点滅させる
    if (_shownTime >= INPUT_DELAY && fmodf(_shownTime, 1.0f) < 0.6f) {
        GameFont::DrawCentered(centerX, top + 370, "SPACE / A でタイトルへ", 0xAAAAAA, GameFont::Size::Small);
    }
}
