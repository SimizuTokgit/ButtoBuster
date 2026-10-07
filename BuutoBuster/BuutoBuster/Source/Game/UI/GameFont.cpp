#include "GameFont.h"
#include "DxLib.h"
#include <cstring>

namespace {
    constexpr unsigned int EDGE_COLOR = 0x101018;

    struct FontSpec {
        int size;
        int thick;
    };

    // { 大きさ px, 太さ } GameFont.h の Size と同じ順
    const FontSpec SPECS[] = {
        { 18, 3 },  // Small
        { 28, 5 },  // Medium
        { 56, 7 },  // Large
        { 96, 9 },  // Huge
        { 15, 2 },  // Tiny
    };

    int g_handles[] = { -1, -1, -1, -1, -1 };

    constexpr int SIZE_COUNT = static_cast<int>(GameFont::Size::Tiny) + 1;
    static_assert(sizeof(SPECS) / sizeof(SPECS[0]) == SIZE_COUNT, "Size を足したら SPECS にも同じ順で足すこと");
    static_assert(sizeof(g_handles) / sizeof(g_handles[0]) == SIZE_COUNT, "Size を足したら g_handles も増やすこと");
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
