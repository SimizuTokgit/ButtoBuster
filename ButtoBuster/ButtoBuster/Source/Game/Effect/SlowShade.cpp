#include "SlowShade.h"
#include "Player.h"
#include "Time.h"

namespace {
    // 画面の光 (-5) と HUD (0) より先に描く
    constexpr int SORTING_ORDER = -10;

    // 暗くする色と、いちばん暗いときの濃さ 0〜1
    constexpr unsigned int SHADE_COLOR = 0x000000;
    constexpr float SHADE_ALPHA = 0.45f;

    // 暗くなりきるまでと、明るく戻りきるまでの秒数
    constexpr float FADE_IN_TIME = 0.15f;
    constexpr float FADE_OUT_TIME = 0.3f;
}

SlowShade::SlowShade() {
    sortingOrder = SORTING_ORDER;
}

void SlowShade::Update(float deltaTime) {
    // ヒットストップで止まっている間も変わっていくよう実時間で数える
    float step = Time::UnscaledDeltaTime();

    bool isSlow = _player && _player->GetOpponentTimeScale() < 1.0f;
    if (isSlow) {
        _amount += step / FADE_IN_TIME;
        if (_amount > 1.0f) _amount = 1.0f;
    }
    else {
        _amount -= step / FADE_OUT_TIME;
        if (_amount < 0.0f) _amount = 0.0f;
    }
}

void SlowShade::Render() {
    if (_amount <= 0.0f) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(SHADE_ALPHA * _amount * 255.0f));
    DrawBox(0, 0, screenWidth, screenHeight, SHADE_COLOR, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
