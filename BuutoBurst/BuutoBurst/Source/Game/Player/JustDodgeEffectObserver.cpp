#include "JustDodgeEffectObserver.h"
#include "EffectManager.h"

namespace {
    // 全体の速さと、ゆっくりにする長さ 長さは実時間の秒
    constexpr float SLOW_SCALE = 0.25f;
    constexpr float SLOW_TIME = 1.0f;

    // かわしたと分かる水色 反撃の弧と同じ色にする
    constexpr unsigned int FLASH_COLOR = 0x60D0FF;

    // 足元に広げる輪の大きさ
    constexpr float RING_RADIUS = 260.0f;
}

void JustDodgeEffectObserver::OnNotify(const JustDodgeEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    effects->SlowMotion(SLOW_SCALE, SLOW_TIME);
    effects->FlashScreen(FLASH_COLOR, 0.3f, 0.25f);

    // ゆっくりになっている間だけ寄り、見切った瞬間を大きく見せる
    effects->ZoomPunch(6.0f, SLOW_TIME);
    effects->PlayShockwave(event.position, RING_RADIUS, GetColorU8(120, 230, 255, 255));
}
