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
#include "Mathf.h"
#include "UiDraw.h"
#include "UiLayer.h"
#include "DxLib.h"
#include <cstdio>

namespace {
    // ----- 結果の画面の見た目 -----

    // 画面を暗くしきるまでの秒数と、いちばん暗いときの濃さ 0〜255
    constexpr float DARKEN_TIME = 2.0f;
    constexpr int DARKNESS = 190;

    // 負けたときの大きな文字の色 (0xRRGGBB)
    constexpr unsigned int GAME_OVER_COLOR = 0xFF6060;

    // 勝ったときの大きな文字 (色は金) が浮かび上がるまでの秒数
    // 勝ったときは結果より先にこの文字だけを出す 結果が出るまでの間は PhaseData の victoryResultDelay
    constexpr float VICTORY_FADE_TIME = 0.4f;

    // ----- 文字を置く高さ -----

    // 大きな文字の上の端 画面の真ん中からこれだけ上
    constexpr int TOP_RAISE = 190;

    // 大きな文字の上の端からの距離と、その行の色 (新記録の文字は金)
    constexpr int PHASE_Y = 130;            // 到達フェーズ
    constexpr unsigned int PHASE_COLOR = 0xFFFFFF;
    constexpr int STATS_Y = 206;            // 撃破と最大コンボ
    constexpr unsigned int STATS_COLOR = 0xE0E0E0;
    constexpr int BEST_Y = 252;             // 最高記録
    constexpr unsigned int BEST_COLOR = 0xC0C0C0;
    constexpr int NEW_RECORD_Y = 298;
    constexpr int PROMPT_Y = 370;           // タイトルへ戻る案内
}

void ResultScreen::Setup(Player* player, PhaseDirector* director) {
    _player = player;
    _director = director;
    sortingOrder = UiLayer::RESULT;
}

void ResultScreen::Update(float deltaTime) {
    if (!_director || !_director->IsResultReady()) return;

    // ヒットストップの最中に倒れても止まらないよう実時間で数える
    _shownTime += Time::UnscaledDeltaTime();

    if (_isRequested || _shownTime < INPUT_DELAY) return;
    if (!InputSystem::Instance().ConfirmPressed()) return;

    _isRequested = true;
    SoundManager::Instance().PlaySE(UiDraw::DECIDE_SE);
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
    float darkness = Mathf::Clamp01(time / DARKEN_TIME);
    UiDraw::FillScreen(0x000000, static_cast<int>(darkness * DARKNESS));

    int top = screenHeight / 2 - TOP_RAISE;

    // 勝ったときは、結果より先に VICTORY だけを浮かび上がらせる
    if (isVictory) {
        float alpha = (VICTORY_FADE_TIME > 0.0f) ? Mathf::Clamp01(time / VICTORY_FADE_TIME) : 1.0f;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
        GameFont::DrawCentered(centerX, top, "VICTORY", UiDraw::GOLD, GameFont::Size::Huge);
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
    GameFont::DrawCentered(centerX, top + PHASE_Y, text, PHASE_COLOR, GameFont::Size::Large);

    snprintf(text, sizeof(text), "撃破  %d      最大コンボ  %d",
        _director->GetKillCount(), _player ? _player->GetMaxCombo() : 0);
    GameFont::DrawCentered(centerX, top + STATS_Y, text, STATS_COLOR, GameFont::Size::Medium);

    snprintf(text, sizeof(text), "%s の最高記録  フェーズ %d", GameMode::GetData().name, _director->GetBestPhase());
    GameFont::DrawCentered(centerX, top + BEST_Y, text, BEST_COLOR, GameFont::Size::Medium);

    if (_director->IsNewRecord()) {
        GameFont::DrawCentered(centerX, top + NEW_RECORD_Y, "NEW RECORD", UiDraw::GOLD, GameFont::Size::Medium);
    }

    // 点滅させる
    if (_shownTime >= INPUT_DELAY && UiDraw::IsBlinkOn(_shownTime)) {
        GameFont::DrawCentered(centerX, top + PROMPT_Y, "SPACE / A でタイトルへ", UiDraw::PROMPT_COLOR, GameFont::Size::Small);
    }
}
