#pragma once
#include "UIImage.h"

class Player;

// ジャスト回避のあと、敵がゆっくりになっている間だけ画面を暗くする
// 暗さ 色 暗くなる速さと戻る速さは .cpp の先頭に並べてある
//
// 景色と粒のあと、画面の光 (ScreenFlash) と HUD の前に描く 文字や数字は暗くならない
class SlowShade : public UIImage {
private:
    const Player* _player = nullptr;

    // 今の暗さ 0〜1 敵がゆっくりの間は 1 へ、戻ったら 0 へ寄せる
    float _amount = 0.0f;

public:
    SlowShade();

    void Setup(const Player* player) { _player = player; }

    void Update(float deltaTime) override;
    void Render() override;
};
