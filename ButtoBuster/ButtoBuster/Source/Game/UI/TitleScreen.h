#pragma once
#include "UIImage.h"

// タイトル画面
// 操作を出し、決定でモード選択へ進む
// 見た目の数値は .cpp の先頭 背景と入力を待つ時間はモード選択と同じもの (UiDraw.h)
class TitleScreen : public UIImage {
private:
    float _timer = 0.0f;
    bool _isRequested = false;
    int _background = -1;

public:
    ~TitleScreen() override;

    void Start() override;
    void Update(float deltaTime) override;
    void Render() override;
};
