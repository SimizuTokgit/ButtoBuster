#include "SlowShade.h"
#include "Player.h"
#include "Time.h"
#include "Mathf.h"
#include "UiDraw.h"
#include "UiLayer.h"

namespace {
    // 暗くする色と、いちばん暗いときの濃さ 0〜1
    constexpr unsigned int SHADE_COLOR = 0x000000;
    constexpr float SHADE_ALPHA = 0.45f;

    // 暗くなりきるまでと、明るく戻りきるまでの秒数
    constexpr float FADE_IN_TIME = 0.15f;
    constexpr float FADE_OUT_TIME = 0.3f;
}

SlowShade::SlowShade() {
    sortingOrder = UiLayer::SLOW_SHADE;
}

void SlowShade::Update(float deltaTime) {
    // ヒットストップで止まっている間も変わっていくよう実時間で数える
    float step = Time::UnscaledDeltaTime();

    bool isSlow = _player && _player->GetOpponentTimeScale() < 1.0f;
    float change = isSlow ? step / FADE_IN_TIME : -step / FADE_OUT_TIME;
    _amount = Mathf::Clamp01(_amount + change);
}

void SlowShade::Render() {
    if (_amount <= 0.0f) return;

    UiDraw::FillScreen(SHADE_COLOR, static_cast<int>(SHADE_ALPHA * _amount * 255.0f));
}
