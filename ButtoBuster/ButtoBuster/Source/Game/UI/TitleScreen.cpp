#include "TitleScreen.h"
#include "GameFont.h"
#include "ControlsList.h"
#include "ModeSelectScene.h"
#include "InputSystem.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "DxLib.h"
#include <cmath>

TitleScreen::~TitleScreen() {
    if (_background != -1) DeleteGraph(_background);
}

void TitleScreen::Start() {
    // 描画の一覧に登録するのは親の Start なので必ず呼ぶ
    UIImage::Start();

    _background = LoadGraph("Data/2D/TitleBack.png");
    SoundManager::Instance().CrossfadeBGM("BGM_title", 1.0f);
}

void TitleScreen::Update(float deltaTime) {
    _timer += deltaTime;

    if (_isRequested || _timer < INPUT_DELAY) return;
    if (!InputSystem::Instance().ConfirmPressed()) return;

    _isRequested = true;
    SoundManager::Instance().PlaySE("Game/SE_ButtonPush");
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
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawBox(0, 0, screenWidth, screenHeight, 0x000000, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else {
        DrawBox(0, 0, screenWidth, screenHeight, 0x0C0C14, TRUE);
    }

    GameFont::DrawCentered(centerX, 110, "BUTTO BUSTER", 0xFFFFFF, GameFont::Size::Huge);
    GameFont::DrawCentered(centerX, 220, "押し寄せる敵の波を 何フェーズ生き残れるか", 0xD0D0D0, GameFont::Size::Small);

    // 最高記録は難易度ごとに分けたので、モード選択のボタンの横に出す

    // 操作の一覧 (ControlsList.h) 操作 ボタン 補足の 3 列
    // 文字の幅がばらばらのフォントなので、空白ではそろわない 列ごとに x を決めて描く
    // 列の左の端 一覧の左の端からの距離
    constexpr int COLUMN_X[] = { 0, 170, 330 };

    int lineHeight = GameFont::GetHeight(GameFont::Size::Small) + 8;
    int left = centerX - 300;
    int top = 320;
    for (int i = 0; i < ControlsList::ROW_COUNT; ++i) {
        const ControlsList::Row& row = ControlsList::ROWS[i];
        const char* columns[] = { row.name, row.keys, row.note };
        for (int column = 0; column < 3; ++column) {
            if (columns[column][0] == '\0') continue;
            GameFont::Draw(left + COLUMN_X[column], top + lineHeight * i, columns[column], 0xE8E8E8, GameFont::Size::Small);
        }
    }

    // 点滅させる
    if (fmodf(_timer, 1.0f) < 0.6f) {
        GameFont::DrawCentered(centerX, screenHeight - 80, "PRESS SPACE / A BUTTON", 0xAAAAAA, GameFont::Size::Medium);
    }
}
