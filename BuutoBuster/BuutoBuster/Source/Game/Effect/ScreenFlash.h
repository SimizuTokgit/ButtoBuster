#pragma once
#include "UIImage.h"

// 画面全体を一瞬だけ色で覆う
// 重い一撃は白く 被弾は赤く 全回復は緑に光らせる
//
// フェーズやコンボの表示より先に描き、光っている間も数字が読めるようにする
class ScreenFlash : public UIImage {
private:
    unsigned int _color = 0xFFFFFF;
    float _alpha = 0.0f;
    float _timer = 0.0f;
    float _duration = 0.0f;

public:
    ScreenFlash();

    // 濃さは 0 から 1 光っている途中なら濃いほうを採る
    void Flash(unsigned int color, float alpha, float seconds);

    void Update(float deltaTime) override;
    void Render() override;

private:
    float GetCurrentAlpha() const;
};
