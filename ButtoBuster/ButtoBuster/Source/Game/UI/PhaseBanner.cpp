#include "PhaseBanner.h"
#include "GameFont.h"
#include "PhaseDirector.h"
#include "EnemyData.h"
#include "UiDraw.h"
#include "UiLayer.h"
#include "DxLib.h"
#include <cstdio>

namespace {
    // ----- 置く場所 -----

    // 文字を置く高さの基準 画面の真ん中からこれだけ上
    constexpr int CENTER_RAISE = 60;

    // 文字の後ろに敷く黒い帯 帯の真ん中は基準からこれだけ下 濃さは 0〜255
    constexpr int BAND_OFFSET = 30;
    constexpr int BAND_ALPHA = 150;

    // 出てから消えるまでの濃さ 最初のこの秒数で出して、最後のこの秒数で消す
    constexpr float FADE_TIME = 0.25f;

    // ----- フェーズの開始 -----
    // 文字の上の端は基準からの距離
    constexpr int ANNOUNCE_BAND_HEIGHT = 170;
    constexpr int ANNOUNCE_TITLE_Y = -20;
    constexpr unsigned int ANNOUNCE_TITLE_COLOR = 0xFFFFFF;
    constexpr int NEW_ENEMY_Y = 76;
    constexpr unsigned int NEW_ENEMY_COLOR = 0xFFB060;

    // ----- フェーズのクリア -----
    constexpr unsigned int CLEAR_COLOR = 0xFFE070;

    // ----- 全回復 -----
    constexpr int REST_BAND_HEIGHT = 150;
    constexpr int REST_TITLE_Y = -10;
    constexpr unsigned int REST_TITLE_COLOR = 0x80FFA0;
    constexpr int REST_TEXT_Y = 56;
    constexpr unsigned int REST_TEXT_COLOR = 0xE0FFE8;

    float FadeAlpha(float time, float duration) {
        if (time < FADE_TIME) return time / FADE_TIME;
        if (time > duration - FADE_TIME) return (duration - time) / FADE_TIME;
        return 1.0f;
    }

    void DrawBand(int screenWidth, int centerY, int height, float alpha) {
        UiDraw::FillBox(0, centerY - height / 2, screenWidth, centerY + height / 2, 0x000000,
            static_cast<int>(alpha * BAND_ALPHA));
    }
}

void PhaseBanner::Setup(PhaseDirector* director) {
    _director = director;
    sortingOrder = UiLayer::PHASE_BANNER;
}

void PhaseBanner::Render() {
    if (!_director) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2 - CENTER_RAISE;

    // 表示の長さは、進め方と同じ PhaseData の時間に合わせる
    const PhaseData& phase = _director->data;
    float time = _director->GetStepTimer();
    char text[64];

    switch (_director->GetStep()) {
    case PhaseDirector::Step::Announce: {
        float alpha = FadeAlpha(time, phase.announceTime);
        if (alpha <= 0.0f) break;

        DrawBand(screenWidth, centerY + BAND_OFFSET, ANNOUNCE_BAND_HEIGHT, alpha);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));

        snprintf(text, sizeof(text), "PHASE %d", _director->GetPhase());
        GameFont::DrawCentered(centerX, centerY + ANNOUNCE_TITLE_Y, text, ANNOUNCE_TITLE_COLOR, GameFont::Size::Huge);

        if (const char* newEnemy = FindNewEnemyName(_director->GetPhase())) {
            snprintf(text, sizeof(text), "新しい敵  %s", newEnemy);
            GameFont::DrawCentered(centerX, centerY + NEW_ENEMY_Y, text, NEW_ENEMY_COLOR, GameFont::Size::Medium);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        break;
    }

    case PhaseDirector::Step::Clear: {
        float alpha = FadeAlpha(time, phase.clearTime);
        if (alpha <= 0.0f) break;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
        GameFont::DrawCentered(centerX, centerY, "PHASE CLEAR", CLEAR_COLOR, GameFont::Size::Large);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        break;
    }

    case PhaseDirector::Step::Rest: {
        float alpha = FadeAlpha(time, phase.restTime);
        if (alpha <= 0.0f) break;

        DrawBand(screenWidth, centerY + BAND_OFFSET, REST_BAND_HEIGHT, alpha);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
        GameFont::DrawCentered(centerX, centerY + REST_TITLE_Y, "FULL HEAL", REST_TITLE_COLOR, GameFont::Size::Large);
        GameFont::DrawCentered(centerX, centerY + REST_TEXT_Y, "バースト値が 0 に戻った 次の波まで少し休める", REST_TEXT_COLOR, GameFont::Size::Small);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        break;
    }

    default:
        break;
    }
}

const char* PhaseBanner::FindNewEnemyName(int phase) const {
    for (int i = 0; i < static_cast<int>(EnemyKind::Count); ++i) {
        const EnemyData& data = EnemyDatabase::Get(static_cast<EnemyKind>(i));
        // 最初のフェーズの Goblin はわざわざ出さない
        if (data.unlockPhase == phase && phase > 1) return data.displayName;
    }
    return nullptr;
}
