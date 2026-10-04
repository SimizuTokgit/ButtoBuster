#include "PhaseBanner.h"
#include "GameFont.h"
#include "PhaseDirector.h"
#include "EnemyData.h"
#include "DxLib.h"
#include <cstdio>

namespace {
    // 出てから消えるまでの濃さ 最初の少しで出して、最後の少しで消す
    float FadeAlpha(float time, float duration) {
        constexpr float FADE = 0.25f;
        if (time < FADE) return time / FADE;
        if (time > duration - FADE) return (duration - time) / FADE;
        return 1.0f;
    }

    void DrawBand(int screenWidth, int centerY, int height, float alpha) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 150.0f));
        DrawBox(0, centerY - height / 2, screenWidth, centerY + height / 2, 0x000000, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void PhaseBanner::Setup(PhaseDirector* director) {
    _director = director;
    sortingOrder = 10;
}

void PhaseBanner::Render() {
    if (!_director) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2 - 60;

    // 表示の長さは、進め方と同じ PhaseData の時間に合わせる
    const PhaseData& phase = _director->data;
    float time = _director->GetStepTimer();
    char text[64];

    switch (_director->GetStep()) {
    case PhaseDirector::Step::Announce: {
        float alpha = FadeAlpha(time, phase.announceTime);
        if (alpha <= 0.0f) break;

        DrawBand(screenWidth, centerY + 30, 170, alpha);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));

        snprintf(text, sizeof(text), "PHASE %d", _director->GetPhase());
        GameFont::DrawCentered(centerX, centerY - 20, text, 0xFFFFFF, GameFont::Size::Huge);

        if (const char* newEnemy = FindNewEnemyName(_director->GetPhase())) {
            snprintf(text, sizeof(text), "新しい敵  %s", newEnemy);
            GameFont::DrawCentered(centerX, centerY + 76, text, 0xFFB060, GameFont::Size::Medium);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        break;
    }

    case PhaseDirector::Step::Clear: {
        float alpha = FadeAlpha(time, phase.clearTime);
        if (alpha <= 0.0f) break;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
        GameFont::DrawCentered(centerX, centerY, "PHASE CLEAR", 0xFFE070, GameFont::Size::Large);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        break;
    }

    case PhaseDirector::Step::Rest: {
        float alpha = FadeAlpha(time, phase.restTime);
        if (alpha <= 0.0f) break;

        DrawBand(screenWidth, centerY + 30, 150, alpha);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
        GameFont::DrawCentered(centerX, centerY - 10, "FULL HEAL", 0x80FFA0, GameFont::Size::Large);
        GameFont::DrawCentered(centerX, centerY + 56, "体力が全回復した 次の波まで少し休める", 0xE0FFE8, GameFont::Size::Small);
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
