#include "TitleScreen.h"
#include "GameFont.h"
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

    // 操作の一覧 { 1 列目, 2 列目, 3 列目 } 空の行は 1 行あける
    // 文字の幅がばらばらのフォントなので、空白ではそろわない 列ごとに x を決めて描く
    struct ControlRow {
        const char* columns[3];
    };
    const ControlRow controls[] = {
        { { "移動",              "WASD / 左スティック", "" } },
        { { "攻撃",              "J,左クリック / X",    "3 段までつながる" } },
        { { "ガード",            "K / L2",              "押している間 正面を守る" } },
        { { "ジャンプ",          "SPACE / A",           "" } },
        { { "",                  "",                    "" } },
        { { "攻撃 + ジャンプ",   "対空斬り",            "" } },
        { { "攻撃 + ガード",     "溜め斬り",            "R2  押し続けて溜める" } },
        { { "ガード + ジャンプ", "回避",                "L1  一瞬だけ無敵" } },
    };

    // 列の左の端 一覧の左の端からの距離
    constexpr int COLUMN_X[] = { 0, 170, 330 };

    int lineHeight = GameFont::GetHeight(GameFont::Size::Small) + 8;
    int left = centerX - 300;
    int top = 340;
    for (int i = 0; i < static_cast<int>(sizeof(controls) / sizeof(controls[0])); ++i) {
        for (int column = 0; column < 3; ++column) {
            const char* text = controls[i].columns[column];
            if (text[0] == '\0') continue;
            GameFont::Draw(left + COLUMN_X[column], top + lineHeight * i, text, 0xE8E8E8, GameFont::Size::Small);
        }
    }

    // 点滅させる
    if (fmodf(_timer, 1.0f) < 0.6f) {
        GameFont::DrawCentered(centerX, screenHeight - 80, "PRESS SPACE / A BUTTON", 0xAAAAAA, GameFont::Size::Medium);
    }
}
