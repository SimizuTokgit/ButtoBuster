#pragma once

// 画面の文字
// SetFontSize は呼ぶたびにフォントを作り直して重いので、
// 大きさごとのフォントを最初に1回だけ作って使い回す
namespace GameFont {

    // 足した順に並べてある 大きさは GameFont.cpp の SPECS で、同じ順に並べる
    enum class Size {
        Small,
        Medium,
        Large,
        Huge,
        Tiny,       // Small より小さい チュートリアルの説明に使う
    };

    int Get(Size size);

    // 縁取りつき 3D の画面の上に置いても読めるように
    void Draw(int x, int y, const char* text, unsigned int color, Size size);
    void DrawCentered(int centerX, int y, const char* text, unsigned int color, Size size);
    void DrawRight(int rightX, int y, const char* text, unsigned int color, Size size);

    int GetWidth(const char* text, Size size);
    int GetHeight(Size size);
}
