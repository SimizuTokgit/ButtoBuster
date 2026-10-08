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
    SoundManager::Instance().PlaySE("Common/system_enter");
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

    const char* controls[] = {
        "移動              WASD / 左スティック",
        "攻撃              J,左クリック / X  3 段まで繋がる",
        "ガード            K / L2       押している間 正面を守る",
        "ジャンプ          SPACE / A",
        "",
		"攻撃 + ジャンプ   対空斬り   ",
        "攻撃 + ガード     溜め斬り     R2  押し続けて溜める",
        "ガード + ジャンプ 回避         L1  一瞬だけ無敵",
    };

    int lineHeight = GameFont::GetHeight(GameFont::Size::Small) + 8;
    int left = centerX - 300;
    int top = 340;
    for (int i = 0; i < static_cast<int>(sizeof(controls) / sizeof(controls[0])); ++i) {
        GameFont::Draw(left, top + lineHeight * i, controls[i], 0xE8E8E8, GameFont::Size::Small);
    }

    // 点滅させる
    if (fmodf(_timer, 1.0f) < 0.6f) {
        GameFont::DrawCentered(centerX, screenHeight - 80, "PRESS SPACE / A BUTTON", 0xAAAAAA, GameFont::Size::Medium);
    }
}
