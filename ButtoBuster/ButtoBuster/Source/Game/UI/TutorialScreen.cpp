#include "TutorialScreen.h"
#include "TutorialDirector.h"
#include "GameFont.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

namespace {
    // ----- 指示の枠 (画面の上 左上のゲージの右) -----

    // 枠の上の端 右上のスキップの案内より下にしてある
    constexpr int PANEL_TOP = 70;

    // 枠を置く横の範囲 左上の回避ゲージとバースターゲージ (x 400 ほどまで) の右から、画面の右の端の手前まで
    // 枠はこの範囲の真ん中に置く
    constexpr int PANEL_AREA_LEFT = 420;
    constexpr int PANEL_AREA_RIGHT_MARGIN = 20;

    // 枠の幅 説明の行がこれより長いときだけ、その行に合わせて広げる
    constexpr int PANEL_WIDTH = 700;

    // 枠の内側の余白
    constexpr int PANEL_PADDING = 12;

    // 枠の後ろに敷く黒の濃さ 0〜255 3D の上でも読みやすくする
    constexpr int PANEL_SHADE_ALPHA = 170;

    // 枠の線の色 できたときは CLEAR_COLOR になる
    constexpr unsigned int PANEL_LINE_COLOR = 0x606060;

    // 文字の大きさ 見出し (と「OK!」「1 / 3」) と、それ以外 (「STEP 3 / 17」、説明、決定を待つ文字)
    // 行の高さは文字の大きさから決まるので、変えるのはここだけでよい
    constexpr GameFont::Size TITLE_SIZE = GameFont::Size::Small;   // 18px
    constexpr GameFont::Size TEXT_SIZE = GameFont::Size::Tiny;     // 15px

    // 行と行のすき間
    constexpr int STEP_GAP = 2;     // 「STEP 3 / 17」と見出しの間
    constexpr int TITLE_GAP = 6;    // 見出しと説明の間
    constexpr int LINE_GAP = 4;     // 説明の行どうしの間

    // 見出しと、右の「1 / 3」や進み具合のバーとの間 枠を広げるときに空けておく
    constexpr int TITLE_SIDE_GAP = 24;

    constexpr unsigned int STEP_COLOR = 0xA0A0A0;      // 「STEP 3 / 17」
    constexpr unsigned int TITLE_COLOR = 0xFFFFFF;     // 見出し
    constexpr unsigned int LINE_COLOR = 0xE0E0E0;      // 説明
    constexpr unsigned int COUNT_COLOR = 0xFFD060;     // 「1 / 3」と進み具合のバー
    constexpr unsigned int CLEAR_COLOR = 0x80FF90;     // できたときの「OK!」
    constexpr unsigned int CONFIRM_COLOR = 0xAAAAAA;   // 決定を待つ文字

    // 進み具合のバーの大きさ 見出しの高さの真ん中に置く
    constexpr int BAR_WIDTH = 160;
    constexpr int BAR_HEIGHT = 8;

    // 決定を待つ文字 最後の段だけ本番へ進む
    constexpr const char* CONFIRM_TEXT = "SPACE / 左クリック / A で次へ";
    constexpr const char* FINISH_TEXT = "SPACE / 左クリック / A で本番へ";

    // ----- 長押しでスキップ (右上) -----

    constexpr int SKIP_RIGHT_MARGIN = 40;
    constexpr int SKIP_TOP = 26;
    constexpr GameFont::Size SKIP_SIZE = GameFont::Size::Tiny;
    constexpr int SKIP_BAR_WIDTH = 200;
    constexpr int SKIP_BAR_HEIGHT = 5;
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

    // 説明の行の数と、いちばん長い行の幅 文字は折り返さないので、長い行に合わせて枠を広げる
    int lineCount = 0;
    int contentWidth = GameFont::GetWidth(step.title, TITLE_SIZE) + TITLE_SIDE_GAP + BAR_WIDTH;
    for (const char* line : step.lines) {
        if (!line || line[0] == '\0') continue;
        lineCount++;
        int width = GameFont::GetWidth(line, TEXT_SIZE);
        if (width > contentWidth) contentWidth = width;
    }
    int panelWidth = contentWidth + PANEL_PADDING * 2;
    if (panelWidth < PANEL_WIDTH) panelWidth = PANEL_WIDTH;

    int textHeight = GameFont::GetHeight(TEXT_SIZE);
    int titleHeight = GameFont::GetHeight(TITLE_SIZE);
    int lineStep = textHeight + LINE_GAP;

    int areaWidth = screenWidth - PANEL_AREA_RIGHT_MARGIN - PANEL_AREA_LEFT;
    int left = PANEL_AREA_LEFT + (areaWidth - panelWidth) / 2;
    int right = left + panelWidth;
    int stepY = PANEL_TOP + PANEL_PADDING;
    int titleY = stepY + textHeight + STEP_GAP;
    int linesY = titleY + titleHeight + TITLE_GAP;

    // 決定を待つ段は、点滅させる文字の分も先に空けておく 点滅で枠の大きさが変わらないように
    int rowCount = lineCount + (_director->IsReadStep() ? 1 : 0);
    int bottom = linesY + lineStep * rowCount - LINE_GAP + PANEL_PADDING;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, PANEL_SHADE_ALPHA);
    DrawBox(left, PANEL_TOP, right, bottom, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(left, PANEL_TOP, right, bottom, isCleared ? CLEAR_COLOR : PANEL_LINE_COLOR, FALSE);

    char text[32];
    snprintf(text, sizeof(text), "STEP %d / %d", _director->GetStepNumber(), _director->GetStepCount());
    GameFont::Draw(left + PANEL_PADDING, stepY, text, STEP_COLOR, TEXT_SIZE);

    GameFont::Draw(left + PANEL_PADDING, titleY, step.title, isCleared ? CLEAR_COLOR : TITLE_COLOR, TITLE_SIZE);

    // 見出しの右に、できた印か進み具合
    int progressRight = right - PANEL_PADDING;
    if (isCleared) {
        GameFont::DrawRight(progressRight, titleY, "OK!", CLEAR_COLOR, TITLE_SIZE);
    }
    else if (_director->IsCounted()) {
        snprintf(text, sizeof(text), "%d / %d", _director->GetProgress(), step.count);
        GameFont::DrawRight(progressRight, titleY, text, COUNT_COLOR, TITLE_SIZE);
    }
    else if (_director->HasProgressBar()) {
        int barLeft = progressRight - BAR_WIDTH;
        int barTop = titleY + (titleHeight - BAR_HEIGHT) / 2;
        int fill = static_cast<int>(BAR_WIDTH * _director->GetProgressRatio());
        DrawBox(barLeft, barTop, barLeft + fill, barTop + BAR_HEIGHT, COUNT_COLOR, TRUE);
        DrawBox(barLeft, barTop, progressRight, barTop + BAR_HEIGHT, 0xFFFFFF, FALSE);
    }

    int y = linesY;
    for (const char* line : step.lines) {
        if (!line || line[0] == '\0') continue;
        GameFont::Draw(left + PANEL_PADDING, y, line, LINE_COLOR, TEXT_SIZE);
        y += lineStep;
    }

    // 読む段は、決定を待つ文字を点滅させる
    if (isWaiting && fmodf(_director->GetStepTime(), 1.0f) < 0.6f) {
        const char* confirm = (step.goal == TutorialGoal::Finish) ? FINISH_TEXT : CONFIRM_TEXT;
        GameFont::DrawRight(progressRight, y, confirm, CONFIRM_COLOR, TEXT_SIZE);
    }
}

void TutorialScreen::DrawSkip(int screenWidth) {
    int right = screenWidth - SKIP_RIGHT_MARGIN;
    int textHeight = GameFont::GetHeight(SKIP_SIZE);
    int barTop = SKIP_TOP + textHeight + 5;
    int textWidth = GameFont::GetWidth(SKIP_TEXT, SKIP_SIZE);
    int shadeLeft = right - ((textWidth > SKIP_BAR_WIDTH) ? textWidth : SKIP_BAR_WIDTH) - 8;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, SKIP_SHADE_ALPHA);
    DrawBox(shadeLeft, SKIP_TOP - 5, right + 8, barTop + SKIP_BAR_HEIGHT + 5, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    GameFont::DrawRight(right, SKIP_TOP, SKIP_TEXT, SKIP_COLOR, SKIP_SIZE);

    // 押している間だけ、溜まり具合のバーを伸ばす
    int barLeft = right - SKIP_BAR_WIDTH;
    int fill = static_cast<int>(SKIP_BAR_WIDTH * _director->GetSkipRatio());
    if (fill > 0) DrawBox(barLeft, barTop, barLeft + fill, barTop + SKIP_BAR_HEIGHT, SKIP_BAR_COLOR, TRUE);
    DrawBox(barLeft, barTop, right, barTop + SKIP_BAR_HEIGHT, 0x808080, FALSE);
}
