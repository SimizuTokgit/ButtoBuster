#include "VictoryEffectObserver.h"
#include "EffectManager.h"
#include "StageBuilder.h"

namespace {
    // どれも 0 にすると、その演出は出さない

    // 画面を光らせる色 (0xRRGGBB) と濃さと長さ 秒
    constexpr unsigned int FLASH_COLOR = 0xFFE8A0;
    constexpr float FLASH_ALPHA = 0.5f;
    constexpr float FLASH_TIME = 0.6f;

    // 画面の揺れの強さと長さ 秒 最後の 1 体のとどめで大きく揺らした後なので、軽くにしてある
    constexpr float SHAKE = 5.0f;
    constexpr float SHAKE_TIME = 0.3f;

    // 一瞬寄る角度 度 と、戻るまでの長さ 秒
    constexpr float ZOOM = 6.0f;
    constexpr float ZOOM_TIME = 1.0f;

    // プレイヤーの足元に広げる輪の大きさと色 (0xRRGGBB)
    constexpr float SHOCKWAVE_RADIUS = 900.0f;
    constexpr unsigned int SHOCKWAVE_COLOR = 0xFFD060;

    COLOR_U8 ToColor(unsigned int rgb) {
        return GetColorU8((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255);
    }
}

void VictoryEffectObserver::OnNotify(const VictoryEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    effects->FlashScreen(FLASH_COLOR, FLASH_ALPHA, FLASH_TIME);
    effects->Shake(SHAKE, SHAKE_TIME);
    effects->ZoomPunch(ZOOM, ZOOM_TIME);

    // 跳んでいても、輪は真下の地面に出す
    VECTOR ground = event.playerPosition;
    StageBuilder::FindGroundHeight(ground.x, ground.z, ground.y);
    effects->PlayShockwave(ground, SHOCKWAVE_RADIUS, ToColor(SHOCKWAVE_COLOR));
}
