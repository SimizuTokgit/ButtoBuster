#include "TitleScreen.h"
#include "GameFont.h"
#include "ControlsList.h"
#include "ModeSelectScene.h"
#include "InputSystem.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "UiDraw.h"
#include "DxLib.h"

namespace {
    // ----- 音 -----
    constexpr const char* BGM = "BGM_title";
    constexpr float BGM_FADE_TIME = 1.0f;   // 前の曲から替えるのにかける秒数

    // ----- 見出し -----
    constexpr const char* TITLE_TEXT = "BUTTO BUSTER";
    constexpr int TITLE_Y = 110;
    constexpr unsigned int TITLE_COLOR = 0xFFFFFF;

    constexpr const char* SUBTITLE_TEXT = "押し寄せる敵の波を 何フェーズ生き残れるか";
    constexpr int SUBTITLE_Y = 220;
    constexpr unsigned int SUBTITLE_COLOR = 0xD0D0D0;

    // ----- 操作の一覧 (ControlsList.h) 操作 ボタン 補足の 3 列 -----
    // 文字の幅がばらばらのフォントなので、空白ではそろわない 列ごとに x を決めて描く
    // 列の左の端 一覧の左の端からの距離
    constexpr int COLUMN_X[] = { 0, 170, 330 };
    constexpr int COLUMN_COUNT = static_cast<int>(sizeof(COLUMN_X) / sizeof(COLUMN_X[0]));

    constexpr int CONTROLS_LEFT_OFFSET = -300;  // 一覧の左の端 画面の真ん中からの距離
    constexpr int CONTROLS_TOP = 320;
    constexpr int CONTROLS_LINE_GAP = 8;        // 行どうしの間
    constexpr unsigned int CONTROLS_COLOR = 0xE8E8E8;

    // ----- 始める案内 (画面の下 点滅させる) -----
    constexpr const char* PROMPT_TEXT = "PRESS SPACE / A BUTTON";
    constexpr int PROMPT_BOTTOM_MARGIN = 80;
}

TitleScreen::~TitleScreen() {
    if (_background != -1) DeleteGraph(_background);
}

void TitleScreen::Start() {
    // 描画の一覧に登録するのは親の Start なので必ず呼ぶ
    UIImage::Start();

    _background = LoadGraph(UiDraw::TITLE_BACKGROUND_IMAGE);
    SoundManager::Instance().CrossfadeBGM(BGM, BGM_FADE_TIME);
}

void TitleScreen::Update(float deltaTime) {
    _timer += deltaTime;

    if (_isRequested || _timer < UiDraw::MENU_INPUT_DELAY) return;
    if (!InputSystem::Instance().ConfirmPressed()) return;

    _isRequested = true;
    SoundManager::Instance().PlaySE(UiDraw::DECIDE_SE);
    // 難易度を選ぶ画面へ チュートリアルと本番はそのあと
    SceneManager::Instance().RequestLoadScene<ModeSelectScene>();
}

void TitleScreen::Render() {
    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);
    int centerX = screenWidth / 2;

    if (_background != -1) {
        DrawExtendGraph(0, 0, screenWidth, screenHeight, _background, FALSE);
        UiDraw::FillScreen(0x000000, UiDraw::MENU_SHADE_ALPHA);
    }
    else {
        UiDraw::FillScreen(UiDraw::MENU_NO_IMAGE_COLOR);
    }

    GameFont::DrawCentered(centerX, TITLE_Y, TITLE_TEXT, TITLE_COLOR, GameFont::Size::Huge);
    GameFont::DrawCentered(centerX, SUBTITLE_Y, SUBTITLE_TEXT, SUBTITLE_COLOR, GameFont::Size::Small);

    int lineHeight = GameFont::GetHeight(GameFont::Size::Small) + CONTROLS_LINE_GAP;
    int left = centerX + CONTROLS_LEFT_OFFSET;
    for (int i = 0; i < ControlsList::ROW_COUNT; ++i) {
        const ControlsList::Row& row = ControlsList::ROWS[i];
        const char* columns[COLUMN_COUNT] = { row.name, row.keys, row.note };
        for (int column = 0; column < COLUMN_COUNT; ++column) {
            if (columns[column][0] == '\0') continue;
            GameFont::Draw(left + COLUMN_X[column], CONTROLS_TOP + lineHeight * i, columns[column], CONTROLS_COLOR, GameFont::Size::Small);
        }
    }

    if (UiDraw::IsBlinkOn(_timer)) {
        GameFont::DrawCentered(centerX, screenHeight - PROMPT_BOTTOM_MARGIN, PROMPT_TEXT, UiDraw::PROMPT_COLOR, GameFont::Size::Medium);
    }
}
