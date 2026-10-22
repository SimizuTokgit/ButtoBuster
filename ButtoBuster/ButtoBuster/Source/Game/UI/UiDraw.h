#pragma once
#include "DxLib.h"
#include <cmath>

// 2D の画面で共通に使う描き方と、いくつもの画面で同じ意味に使う値
// 同じ書き方をファイルごとに書かないよう、ここに 1 つだけ置く
namespace UiDraw {
    // ----- 色 (0xRRGGBB) -----

    // 金 数えている数 進み具合のバー 勝ちや新記録など、よいことを知らせる文字
    constexpr unsigned int GOLD = 0xFFD060;

    // 押すボタンの案内の文字 (「SPACE / A で次へ」など)
    constexpr unsigned int PROMPT_COLOR = 0xAAAAAA;

    // ----- 点滅 -----

    // 1 秒ごとに、はじめのこの秒数だけ出す
    constexpr float BLINK_ON_TIME = 0.6f;

    // 点滅させる文字を今出すか seconds は出し始めてからの秒数
    inline bool IsBlinkOn(float seconds) {
        return fmodf(seconds, 1.0f) < BLINK_ON_TIME;
    }

    // ----- 四角 -----

    // 透かした塗りの四角 alpha は 0〜255
    inline void FillBox(int left, int top, int right, int bottom, unsigned int color, int alpha) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(left, top, right, bottom, color, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 画面全体を塗る
    inline void FillScreen(unsigned int color) {
        int screenWidth = 0;
        int screenHeight = 0;
        GetDrawScreenSize(&screenWidth, &screenHeight);
        DrawBox(0, 0, screenWidth, screenHeight, color, TRUE);
    }

    // 画面全体を透かして塗る alpha は 0〜255
    inline void FillScreen(unsigned int color, int alpha) {
        int screenWidth = 0;
        int screenHeight = 0;
        GetDrawScreenSize(&screenWidth, &screenHeight);
        FillBox(0, 0, screenWidth, screenHeight, color, alpha);
    }

    // 枠つきのバー 左から ratio (0〜1) の分だけ塗り、枠はいつも描く
    inline void DrawBar(int left, int top, int width, int height, float ratio, unsigned int fillColor, unsigned int lineColor) {
        int fill = static_cast<int>(width * ratio);
        if (fill > 0) DrawBox(left, top, left + fill, top + height, fillColor, TRUE);
        DrawBox(left, top, left + width, top + height, lineColor, FALSE);
    }

    // ----- タイトルとモード選択の画面 -----

    // 背景の画像 モード選択では、画像が無い難易度ともどるボタンで使う
    constexpr const char* TITLE_BACKGROUND_IMAGE = "Data/2D/TitleBack.png";

    // 背景の上に敷く黒の濃さ 0〜255 文字を読みやすくする
    constexpr int MENU_SHADE_ALPHA = 120;

    // 背景の画像が無いときの色
    constexpr unsigned int MENU_NO_IMAGE_COLOR = 0x0C0C14;

    // 前の画面で押しっぱなしのボタンで、すぐに決まらないように少し待つ 秒
    constexpr float MENU_INPUT_DELAY = 0.3f;

    // 決定の音
    constexpr const char* DECIDE_SE = "Game/SE_ButtonPush";
}
