#pragma once
#include "UIImage.h"

// タイトル画面
// 最高記録と操作を出し、決定でゲームへ進む
class TitleScreen : public UIImage {
private:
    // 前の画面で押しっぱなしのボタンで即開始しないように少し待つ
    static constexpr float INPUT_DELAY = 0.3f;

    float _timer = 0.0f;
    bool _isRequested = false;
    int _bestPhase = 0;
    int _background = -1;

public:
    ~TitleScreen() override;

    void Start() override;
    void Update(float deltaTime) override;
    void Render() override;
};
