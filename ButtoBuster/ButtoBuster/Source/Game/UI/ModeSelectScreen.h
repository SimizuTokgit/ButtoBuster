#pragma once
#include "UIImage.h"
#include "Difficulty.h"

// モード選択の画面
// EASY NORMAL HARD もどる のボタンを縦に並べ、選んでいるボタンで背景を替える
// 難易度を決めるとチュートリアルへ、もどるかキャンセルでタイトルへ戻る
// 見た目と操作の数値は ModeSelectScreen.cpp の先頭
class ModeSelectScreen : public UIImage {
private:
    // ボタンの数 難易度の数 + もどる
    static constexpr int BUTTON_COUNT = static_cast<int>(Difficulty::Count) + 1;
    static constexpr int BACK_INDEX = BUTTON_COUNT - 1;

    float _timer = 0.0f;
    bool _isRequested = false;

    int _focus = 0;

    // 背景の画像 ボタンごとに 1 枚 読めなかったボタンはタイトルの背景を使う
    int _backgrounds[BUTTON_COUNT] = {};
    int _titleBackground = -1;

    // 背景を切り替えるときは、前の背景から重ねて替える
    int _previousFocus = 0;
    float _fadeTimer = 0.0f;

    int _bestPhases[static_cast<int>(Difficulty::Count)] = {};

    int _lastMouseX = -1;
    int _lastMouseY = -1;

public:
    ModeSelectScreen() {
        for (int& handle : _backgrounds) handle = -1;
    }
    ~ModeSelectScreen() override;

    void Start() override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    void MoveFocus(int step);
    void SetFocus(int index);
    void Decide();

    // マウスのカーソルが乗っているボタン 乗っていなければ -1
    int ButtonAt(int x, int y) const;

    // ボタンの位置 画面の大きさから決める
    void GetButtonRect(int index, int* left, int* top, int* right, int* bottom) const;

    int GetBackground(int index) const;
    void DrawBackground(int handle, int alpha, int screenWidth, int screenHeight) const;
};
