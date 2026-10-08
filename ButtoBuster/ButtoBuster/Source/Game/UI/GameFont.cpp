#include "GameFont.h"
#include "DxLib.h"
#include <cstring>

namespace {
    // ----- 使うフォント -----
    // ファイルはゲームの中だけで使えるように読み込む (パソコンに入れなくてよい)
    // 名前はフォントの中に書いてある名前 (叛逆明朝 の英語の名前) 読めなかったときは標準のフォントで描く
    constexpr const char* FONT_FILE = "Data/Font/Hangyaku.ttf";
    constexpr const char* FONT_NAME = "Hangyaku";

    constexpr unsigned int EDGE_COLOR = 0x101018;

    struct FontSpec {
        int size;
        int thick;
        bool useFontFile;   // true で FONT_FILE のフォント、false で標準のフォント
    };

    // { 大きさ px, 太さ, FONT_FILE を使うか } GameFont.h の Size と同じ順
    // 叛逆明朝 は字が細く縦長で小さいと読みにくいので、見出しの大きさ (Medium Large Huge) だけに使う
    const FontSpec SPECS[] = {
        { 18, 3, false },   // Small  説明の文字 読みやすさを優先して標準のフォント
        { 28, 5, true },    // Medium
        { 56, 7, true },    // Large
        { 96, 9, true },    // Huge
        { 15, 2, false },   // Tiny   チュートリアルの説明 同じく標準のフォント
    };

    int g_handles[] = { -1, -1, -1, -1, -1 };

    // フォントのファイルを読み込んだか 最初に文字を作るときに 1 回だけ読む
    bool g_isFontLoaded = false;
    bool g_hasFontFile = false;

    // FR_PRIVATE で読むので、このゲームを閉じると自動で外れる
    void LoadFontFile() {
        if (g_isFontLoaded) return;
        g_isFontLoaded = true;
        g_hasFontFile = AddFontResourceExA(FONT_FILE, FR_PRIVATE, nullptr) > 0;
    }

    constexpr int SIZE_COUNT = static_cast<int>(GameFont::Size::Tiny) + 1;
    static_assert(sizeof(SPECS) / sizeof(SPECS[0]) == SIZE_COUNT, "Size を足したら SPECS にも同じ順で足すこと");
    static_assert(sizeof(g_handles) / sizeof(g_handles[0]) == SIZE_COUNT, "Size を足したら g_handles も増やすこと");
}

int GameFont::Get(Size size) {
    int index = static_cast<int>(size);
    if (g_handles[index] == -1) {
        LoadFontFile();
        const FontSpec& spec = SPECS[index];
        g_handles[index] = CreateFontToHandle((g_hasFontFile && spec.useFontFile) ? FONT_NAME : nullptr, spec.size, spec.thick, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 2);
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
