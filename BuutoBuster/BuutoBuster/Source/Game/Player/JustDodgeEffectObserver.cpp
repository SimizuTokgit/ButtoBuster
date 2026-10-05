#include "JustDodgeEffectObserver.h"
#include "EffectManager.h"

namespace {
    // 見切った瞬間に全体を止める秒数 プレイヤーは寄って反撃するので、長くは止めない
    constexpr float STOP_TIME = 0.1f;

    // 寄る角度と、寄っている長さ 秒
    constexpr float ZOOM_DEGREE = 6.0f;
    constexpr float ZOOM_TIME = 0.5f;

    // かわしたと分かる水色 反撃の弧と同じ色にする
    constexpr unsigned int FLASH_COLOR = 0x60D0FF;

    // 足元に広げる輪の大きさ
    constexpr float RING_RADIUS = 260.0f;
}

void JustDodgeEffectObserver::OnNotify(const JustDodgeEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    effects->HitStop(STOP_TIME);
    effects->FlashScreen(FLASH_COLOR, 0.3f, 0.25f);

    // 見切った瞬間を大きく見せる
    effects->ZoomPunch(ZOOM_DEGREE, ZOOM_TIME);
    effects->PlayShockwave(event.position, RING_RADIUS, GetColorU8(120, 230, 255, 255));
}
