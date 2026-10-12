#include "ModeSelectScreen.h"
#include "GameFont.h"
#include "SaveData.h"
#include "TitleScene.h"
#include "TutorialScene.h"
#include "InputSystem.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "Mathf.h"
#include "UiDraw.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

namespace {
    // ----- 見出し -----
    constexpr const char* HEADING = "MODE SELECT";
    constexpr int HEADING_Y = 90;
    constexpr unsigned int HEADING_COLOR = 0xFFFFFF;

    // ----- ボタン (画面の真ん中に縦に並べる) -----
    constexpr int BUTTON_WIDTH = 460;
    constexpr int BUTTON_HEIGHT = 64;
    constexpr int BUTTON_TOP = 220;         // いちばん上のボタンの上の端
    constexpr int BUTTON_GAP = 16;          // ボタンどうしの間
    constexpr int BACK_EXTRA_GAP = 24;      // もどるボタンの上だけ、さらに空ける
    constexpr int BUTTON_PADDING = 24;      // ボタンの左右の内側の余白

    constexpr const char* BACK_LABEL = "もどる";

    // ボタンごとの色 選んでいるボタンの枠と名前に使う 並びは EASY NORMAL HARD もどる
    constexpr unsigned int ACCENT_COLORS[] = { 0x80FF90, 0xFFD060, 0xFF6060, 0xC0C0C0 };

    constexpr unsigned int BUTTON_COLOR = 0x101018;         // ボタンの地
    constexpr int BUTTON_ALPHA = 200;                       // ボタンの地の濃さ 0〜255
    constexpr unsigned int IDLE_LINE_COLOR = 0x505060;      // 選んでいないボタンの枠
    constexpr unsigned int IDLE_TEXT_COLOR = 0xA0A0A8;      // 選んでいないボタンの文字
    constexpr unsigned int RECORD_COLOR = 0xE0E0E0;         // 最高記録

    // 選んでいるボタンを、左右に少しだけ揺らして目立たせる 0 で揺らさない
    constexpr float FOCUS_SWAY = 4.0f;
    constexpr float FOCUS_SWAY_SPEED = 6.0f;

    // ----- 説明 (ボタンの下) -----
    constexpr int DESCRIPTION_GAP = 28;     // いちばん下のボタンとの間
    constexpr unsigned int DESCRIPTION_COLOR = 0xD0D0D0;

    // ----- 操作の案内 (画面の下) -----
    constexpr const char* HINT_TEXT = "W S / ↑ ↓ / 十字キーで選ぶ    SPACE / A で決定    BackSpace / B でもどる";
    constexpr int HINT_BOTTOM_MARGIN = 60;

    // ----- 背景 -----
    // 画像はタイトルと同じもの (UiDraw.h) と難易度ごとのもの 上に敷く黒と、画像が 1 枚も無いときの色もタイトルと同じ
    constexpr float BACKGROUND_FADE_TIME = 0.25f;   // 背景を替えるときに重ねる時間 秒

    // ----- 音 決定の音はタイトルと同じ -----
    constexpr const char* MOVE_SE = "Game/SE_ButtonSelect";
    constexpr float MOVE_SE_VOLUME = 0.5f;
}

ModeSelectScreen::~ModeSelectScreen() {
    for (int i = 0; i < DIFFICULTY_COUNT; ++i) {
        if (_backgrounds[i] != -1) DeleteGraph(_backgrounds[i]);
    }
    if (_titleBackground != -1) DeleteGraph(_titleBackground);
}

void ModeSelectScreen::Start() {
    // 描画の一覧に登録するのは親の Start なので必ず呼ぶ
    UIImage::Start();

    InputSystem::Instance().SetMouseLook(false);

    _titleBackground = LoadGraph(UiDraw::TITLE_BACKGROUND_IMAGE);
    for (int i = 0; i < DIFFICULTY_COUNT; ++i) {
        Difficulty difficulty = static_cast<Difficulty>(i);
        _backgrounds[i] = LoadGraph(GameMode::GetData(difficulty).background);
        _bestPhases[i] = SaveData::LoadBestPhase(difficulty);
    }

    // 前に選んだ難易度から始める 初めてならノーマル
    _focus = static_cast<int>(GameMode::Get());
    _previousFocus = _focus;
    _fadeTimer = BACKGROUND_FADE_TIME;

    InputSystem::Instance().GetMousePosition(&_lastMouseX, &_lastMouseY);
}

void ModeSelectScreen::Update(float deltaTime) {
    _timer += deltaTime;
    _fadeTimer += deltaTime;

    if (_isRequested || _timer < UiDraw::MENU_INPUT_DELAY) return;

    InputSystem& input = InputSystem::Instance();

    // マウスを動かしたら、乗っているボタンを選ぶ 動かしていないときは、キーで選んだボタンを邪魔しない
    int mouseX = 0;
    int mouseY = 0;
    input.GetMousePosition(&mouseX, &mouseY);
    int hovered = ButtonAt(mouseX, mouseY);
    if (mouseX != _lastMouseX || mouseY != _lastMouseY) {
        if (hovered >= 0) SetFocus(hovered);
        _lastMouseX = mouseX;
        _lastMouseY = mouseY;
    }

    if (input.MenuUpPressed()) MoveFocus(-1);
    if (input.MenuDownPressed()) MoveFocus(1);

    if (input.CancelPressed()) {
        _focus = BACK_INDEX;
        Decide();
        return;
    }

    if (input.ConfirmPressed()) {
        // クリックはボタンの上だけ受け付ける
        if (input.MousePressed(MOUSE_INPUT_LEFT)) {
            if (hovered < 0) return;
            SetFocus(hovered);
        }
        Decide();
    }
}

void ModeSelectScreen::MoveFocus(int step) {
    SetFocus((_focus + step + BUTTON_COUNT) % BUTTON_COUNT);
}

void ModeSelectScreen::SetFocus(int index) {
    if (index == _focus) return;

    _previousFocus = _focus;
    _focus = index;
    _fadeTimer = 0.0f;
    SoundManager::Instance().PlaySE(MOVE_SE, MOVE_SE_VOLUME);
}

void ModeSelectScreen::Decide() {
    _isRequested = true;
    SoundManager::Instance().PlaySE(UiDraw::DECIDE_SE);

    if (_focus == BACK_INDEX) {
        SceneManager::Instance().RequestLoadScene<TitleScene>();
        return;
    }
    GameMode::Set(static_cast<Difficulty>(_focus));

    // 本番の前に、毎回チュートリアルを挟む (長押しで飛ばせる)
    SceneManager::Instance().RequestLoadScene<TutorialScene>();
}

int ModeSelectScreen::ButtonAt(int x, int y) const {
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        int left, top, right, bottom;
        GetButtonRect(i, &left, &top, &right, &bottom);
        if (x >= left && x < right && y >= top && y < bottom) return i;
    }
    return -1;
}

void ModeSelectScreen::GetButtonRect(int index, int* left, int* top, int* right, int* bottom) const {
    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    *left = (screenWidth - BUTTON_WIDTH) / 2;
    *right = *left + BUTTON_WIDTH;
    *top = BUTTON_TOP + index * (BUTTON_HEIGHT + BUTTON_GAP);
    if (index == BACK_INDEX) *top += BACK_EXTRA_GAP;
    *bottom = *top + BUTTON_HEIGHT;
}

int ModeSelectScreen::GetBackground(int index) const {
    if (index >= 0 && index < DIFFICULTY_COUNT && _backgrounds[index] != -1) return _backgrounds[index];
    return _titleBackground;
}

void ModeSelectScreen::DrawBackground(int handle, int alpha, int screenWidth, int screenHeight) const {
    if (handle == -1 || alpha <= 0) return;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawExtendGraph(0, 0, screenWidth, screenHeight, handle, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void ModeSelectScreen::Render() {
    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);
    int centerX = screenWidth / 2;

    // ----- 背景 前の背景の上に、今の背景を少しずつ濃く重ねる -----
    UiDraw::FillScreen(UiDraw::MENU_NO_IMAGE_COLOR);
    float fade = (BACKGROUND_FADE_TIME > 0.0f) ? Mathf::Clamp01(_fadeTimer / BACKGROUND_FADE_TIME) : 1.0f;
    int current = GetBackground(_focus);
    int previous = GetBackground(_previousFocus);
    if (fade < 1.0f && previous != current) DrawBackground(previous, 255, screenWidth, screenHeight);
    DrawBackground(current, static_cast<int>(255 * fade), screenWidth, screenHeight);

    UiDraw::FillScreen(0x000000, UiDraw::MENU_SHADE_ALPHA);

    GameFont::DrawCentered(centerX, HEADING_Y, HEADING, HEADING_COLOR, GameFont::Size::Large);

    // ----- ボタン -----
    static_assert(sizeof(ACCENT_COLORS) / sizeof(ACCENT_COLORS[0]) == BUTTON_COUNT,
        "難易度を足したら ACCENT_COLORS にも色を足すこと (最後はもどる)");

    char text[64];
    int textHeight = GameFont::GetHeight(GameFont::Size::Medium);
    int recordHeight = GameFont::GetHeight(GameFont::Size::Small);
    int lastBottom = 0;

    for (int i = 0; i < BUTTON_COUNT; ++i) {
        int left, top, right, bottom;
        GetButtonRect(i, &left, &top, &right, &bottom);
        lastBottom = bottom;

        bool isFocused = (i == _focus);
        unsigned int accent = ACCENT_COLORS[i];

        if (isFocused) {
            int sway = static_cast<int>(sinf(_timer * FOCUS_SWAY_SPEED) * FOCUS_SWAY);
            left += sway;
            right += sway;
        }

        UiDraw::FillBox(left, top, right, bottom, BUTTON_COLOR, BUTTON_ALPHA);

        unsigned int lineColor = isFocused ? accent : IDLE_LINE_COLOR;
        DrawBox(left, top, right, bottom, lineColor, FALSE);
        if (isFocused) DrawBox(left + 1, top + 1, right - 1, bottom - 1, lineColor, FALSE);

        const char* label = (i == BACK_INDEX) ? BACK_LABEL : GameMode::GetData(static_cast<Difficulty>(i)).name;
        GameFont::Draw(left + BUTTON_PADDING, top + (BUTTON_HEIGHT - textHeight) / 2, label,
            isFocused ? accent : IDLE_TEXT_COLOR, GameFont::Size::Medium);

        // 難易度ごとの最高記録を、ボタンの右に出す
        if (i < DIFFICULTY_COUNT) {
            if (_bestPhases[i] > 0) snprintf(text, sizeof(text), "最高  フェーズ %d", _bestPhases[i]);
            else snprintf(text, sizeof(text), "記録なし");
            GameFont::DrawRight(right - BUTTON_PADDING, top + (BUTTON_HEIGHT - recordHeight) / 2, text,
                isFocused ? RECORD_COLOR : IDLE_TEXT_COLOR, GameFont::Size::Small);
        }
    }

    // ----- 選んでいる難易度の説明 -----
    if (_focus < DIFFICULTY_COUNT) {
        const char* description = GameMode::GetData(static_cast<Difficulty>(_focus)).description;
        GameFont::DrawCentered(centerX, lastBottom + DESCRIPTION_GAP, description, DESCRIPTION_COLOR, GameFont::Size::Small);
    }
    else {
        GameFont::DrawCentered(centerX, lastBottom + DESCRIPTION_GAP, "タイトルへ戻る", DESCRIPTION_COLOR, GameFont::Size::Small);
    }

    GameFont::DrawCentered(centerX, screenHeight - HINT_BOTTOM_MARGIN, HINT_TEXT, UiDraw::PROMPT_COLOR, GameFont::Size::Small);
}
