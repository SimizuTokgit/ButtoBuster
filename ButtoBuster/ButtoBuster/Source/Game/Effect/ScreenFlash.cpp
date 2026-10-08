#include "ScreenFlash.h"
#include "Time.h"

namespace {
    // HUD は 0 番 それより先に描く
    constexpr int SORTING_ORDER = -5;
}

ScreenFlash::ScreenFlash() {
    sortingOrder = SORTING_ORDER;
}

void ScreenFlash::Flash(unsigned int color, float alpha, float seconds) {
    if (alpha <= 0.0f || seconds <= 0.0f) return;
    if (alpha < GetCurrentAlpha()) return;

    _color = color;
    _alpha = alpha;
    _timer = seconds;
    _duration = seconds;
}

void ScreenFlash::Update(float deltaTime) {
    // ヒットストップで止まっている間も消えていくよう実時間で数える
    if (_timer <= 0.0f) return;
    _timer -= Time::UnscaledDeltaTime();
    if (_timer < 0.0f) _timer = 0.0f;
}

void ScreenFlash::Render() {
    float alpha = GetCurrentAlpha();
    if (alpha <= 0.0f) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
    DrawBox(0, 0, screenWidth, screenHeight, _color, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

float ScreenFlash::GetCurrentAlpha() const {
    if (_duration <= 0.0f || _timer <= 0.0f) return 0.0f;

    // 光った瞬間が一番濃く、すぐに引いていく
    float rate = _timer / _duration;
    return _alpha * rate * rate;
}
