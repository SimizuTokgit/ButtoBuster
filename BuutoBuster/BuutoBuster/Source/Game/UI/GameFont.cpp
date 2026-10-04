#include "GameFont.h"
#include "DxLib.h"
#include <cstring>

namespace {
    constexpr unsigned int EDGE_COLOR = 0x101018;

    struct FontSpec {
        int size;
        int thick;
    };

    const FontSpec SPECS[] = {
        { 18, 3 },
        { 28, 5 },
        { 56, 7 },
        { 96, 9 },
    };

    int g_handles[] = { -1, -1, -1, -1 };
}

int GameFont::Get(Size size) {
    int index = static_cast<int>(size);
    if (g_handles[index] == -1) {
        const FontSpec& spec = SPECS[index];
        g_handles[index] = CreateFontToHandle(nullptr, spec.size, spec.thick, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);
    }
    return g_handles[index];
}

void GameFont::Draw(int x, int y, const char* text, unsigned int color, Size size) {
    DrawStringToHandle(x, y, text, color, Get(size), EDGE_COLOR);
}

void GameFont::DrawCentered(int centerX, int y, const char* text, unsigned int color, Size size) {
    Draw(centerX - GetWidth(text, size) / 2, y, text, color, size);
}

void GameFont::DrawRight(int rightX, int y, const char* text, unsigned int color, Size size) {
    Draw(rightX - GetWidth(text, size), y, text, color, size);
}

int GameFont::GetWidth(const char* text, Size size) {
    return GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), Get(size));
}

int GameFont::GetHeight(Size size) {
    return GetFontSizeToHandle(Get(size));
}
