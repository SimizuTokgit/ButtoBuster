#include "TutorialScreen.h"
#include "TutorialDirector.h"
#include "GameFont.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

namespace {
    // ----- 指示の枠 (画面の上の真ん中) -----

    // 枠の上の端と幅 左上の回避のバーと重ならないよう、少し下げてある
    constexpr int PANEL_TOP = 100;
    constexpr int PANEL_WIDTH = 900;

    // 枠の内側の余白
    constexpr int PANEL_PADDING = 18;

    // 枠の後ろに敷く黒の濃さ 0〜255 3D の上でも読みやすくする
    constexpr int PANEL_SHADE_ALPHA = 170;

    // 枠の線の色 できたときは CLEAR_COLOR になる
    constexpr unsigned int PANEL_LINE_COLOR = 0x606060;

    // 「STEP 3 / 17」から見出しまでと、見出しから説明までの間
    constexpr int TITLE_OFFSET = 26;
    constexpr int LINES_OFFSET = 42;

    // 説明の行の間隔
    constexpr int LINE_HEIGHT = 28;

    constexpr unsigned int STEP_COLOR = 0xA0A0A0;      // 「STEP 3 / 17」
    constexpr unsigned int TITLE_COLOR = 0xFFFFFF;     // 見出し
    constexpr unsigned int LINE_COLOR = 0xE0E0E0;      // 説明
    constexpr unsigned int COUNT_COLOR = 0xFFD060;     // 「1 / 3」と進み具合のバー
    constexpr unsigned int CLEAR_COLOR = 0x80FF90;     // できたときの「OK!」
    constexpr unsigned int CONFIRM_COLOR = 0xAAAAAA;   // 決定を待つ文字

    // 進み具合のバーの大きさ
    constexpr int BAR_WIDTH = 200;
    constexpr int BAR_HEIGHT = 10;

    // 決定を待つ文字 最後の段だけ本番へ進む
    constexpr const char* CONFIRM_TEXT = "SPACE / 左クリック / A で次へ";
    constexpr const char* FINISH_TEXT = "SPACE / 左クリック / A で本番へ";

    // ----- 長押しでスキップ (右上) -----

    constexpr int SKIP_RIGHT_MARGIN = 40;
    constexpr int SKIP_TOP = 28;
    constexpr int SKIP_BAR_WIDTH = 240;
    constexpr int SKIP_BAR_HEIGHT = 6;
    constexpr int SKIP_SHADE_ALPHA = 120;
    constexpr const char* SKIP_TEXT = "Enter / Start 長押しでスキップ";
    constexpr unsigned int SKIP_COLOR = 0xD0D0D0;
    constexpr unsigned int SKIP_BAR_COLOR = 0xFFD060;
}

void TutorialScreen::Setup(TutorialDirector* director) {
    _director = director;

    // 戦いの表示 (Hud) より手前に出す
    sortingOrder = 12;
}

void TutorialScreen::Render() {
    if (!_director) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    DrawPanel(screenWidth);
    DrawSkip(screenWidth);
}

void TutorialScreen::DrawPanel(int screenWidth) {
    const TutorialStep& step = _director->GetStep();
    bool isCleared = _director->IsCleared();
    bool isWaiting = _director->IsWaitingConfirm();

    int lineCount = 0;
    for (const char* line : step.lines) {
        if (line && line[0] != '\0') lineCount++;
    }

    int left = (screenWidth - PANEL_WIDTH) / 2;
    int right = left + PANEL_WIDTH;
    int stepY = PANEL_TOP + PANEL_PADDING;
    int titleY = stepY + TITLE_OFFSET;
    int linesY = titleY + LINES_OFFSET;

    // 決定を待つ段は、点滅させる文字の分も先に空けておく 点滅で枠の大きさが変わらないように
    int contentBottom = linesY + LINE_HEIGHT * lineCount;
    if (_director->IsReadStep()) contentBottom += LINE_HEIGHT;
    int bottom = contentBottom + PANEL_PADDING;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, PANEL_SHADE_ALPHA);
    DrawBox(left, PANEL_TOP, right, bottom, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(left, PANEL_TOP, right, bottom, isCleared ? CLEAR_COLOR : PANEL_LINE_COLOR, FALSE);

    char text[32];
    snprintf(text, sizeof(text), "STEP %d / %d", _director->GetStepNumber(), _director->GetStepCount());
    GameFont::Draw(left + PANEL_PADDING, stepY, text, STEP_COLOR, GameFont::Size::Small);

    GameFont::Draw(left + PANEL_PADDING, titleY, step.title, isCleared ? CLEAR_COLOR : TITLE_COLOR, GameFont::Size::Medium);

    // 見出しの右に、できた印か進み具合
    int progressRight = right - PANEL_PADDING;
    if (isCleared) {
        GameFont::DrawRight(progressRight, titleY, "OK!", CLEAR_COLOR, GameFont::Size::Medium);
    }
    else if (_director->IsCounted()) {
        snprintf(text, sizeof(text), "%d / %d", _director->GetProgress(), step.count);
        GameFont::DrawRight(progressRight, titleY, text, COUNT_COLOR, GameFont::Size::Medium);
    }
    else if (_director->HasProgressBar()) {
        int barLeft = progressRight - BAR_WIDTH;
        int barTop = titleY + 10;
        int fill = static_cast<int>(BAR_WIDTH * _director->GetProgressRatio());
        DrawBox(barLeft, barTop, barLeft + fill, barTop + BAR_HEIGHT, COUNT_COLOR, TRUE);
        DrawBox(barLeft, barTop, progressRight, barTop + BAR_HEIGHT, 0xFFFFFF, FALSE);
    }

    int y = linesY;
    for (const char* line : step.lines) {
        if (!line || line[0] == '\0') continue;
        GameFont::Draw(left + PANEL_PADDING, y, line, LINE_COLOR, GameFont::Size::Small);
        y += LINE_HEIGHT;
    }

    // 読む段は、決定を待つ文字を点滅させる
    if (isWaiting && fmodf(_director->GetStepTime(), 1.0f) < 0.6f) {
        const char* confirm = (step.goal == TutorialGoal::Finish) ? FINISH_TEXT : CONFIRM_TEXT;
        GameFont::DrawRight(progressRight, y + 4, confirm, CONFIRM_COLOR, GameFont::Size::Small);
    }
}

void TutorialScreen::DrawSkip(int screenWidth) {
    int right = screenWidth - SKIP_RIGHT_MARGIN;
    int textHeight = GameFont::GetHeight(GameFont::Size::Small);
    int barTop = SKIP_TOP + textHeight + 6;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, SKIP_SHADE_ALPHA);
    DrawBox(right - SKIP_BAR_WIDTH - 8, SKIP_TOP - 6, right + 8, barTop + SKIP_BAR_HEIGHT + 6, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    GameFont::DrawRight(right, SKIP_TOP, SKIP_TEXT, SKIP_COLOR, GameFont::Size::Small);

    // 押している間だけ、溜まり具合のバーを伸ばす
    int barLeft = right - SKIP_BAR_WIDTH;
    int fill = static_cast<int>(SKIP_BAR_WIDTH * _director->GetSkipRatio());
    if (fill > 0) DrawBox(barLeft, barTop, barLeft + fill, barTop + SKIP_BAR_HEIGHT, SKIP_BAR_COLOR, TRUE);
    DrawBox(barLeft, barTop, right, barTop + SKIP_BAR_HEIGHT, 0x808080, FALSE);
}
