#include "SpecialEffectObserver.h"
#include "EffectManager.h"
#include "StageBuilder.h"

namespace {
    // ----- ゲージが満タンになったとき -----
    // 足元に金の輪を広げ、画面を薄く光らせる 撃てるようになったと分かるように
    constexpr float READY_RING_RADIUS = 320.0f;
    constexpr unsigned int READY_COLOR = 0xFFD060;
    constexpr float READY_FLASH_ALPHA = 0.2f;
    constexpr float READY_FLASH_TIME = 0.3f;

    // ----- 剣を掲げて雷を呼ぶ間 -----
    // 画面を暗くして寄る 暗さは呼んだ瞬間がいちばん濃く、雷が落ちるまでに戻っていく
    constexpr unsigned int CALL_SHADE_COLOR = 0x000010;
    constexpr float CALL_SHADE_ALPHA = 0.55f;
    constexpr float CALL_ZOOM = 8.0f;

    // 足元に広げる青い輪
    constexpr float CALL_RING_RADIUS = 450.0f;
    constexpr unsigned int CALL_RING_COLOR = 0x78C8FF;

    // ----- 雷が落ちた瞬間 -----
    // 雷の画像の大きさ (幅と高さ) 画像の下端が、敵の足元に来る
    constexpr float LIGHTNING_WIDTH = 450.0f;
    constexpr float LIGHTNING_HEIGHT = 1100.0f;

    // 雷が出ている秒数と、薄れ始める割合 (出ている長さに対して) と、瞬きの強さ 0〜1
    constexpr float LIGHTNING_LIFE = 0.45f;
    constexpr float LIGHTNING_FADE_START = 0.3f;
    constexpr float LIGHTNING_FLICKER = 0.5f;

    // 雷の色 白なら画像の色のまま
    constexpr unsigned int LIGHTNING_COLOR = 0xFFFFFF;

    // 雷が落ちた足元に広げる輪
    constexpr float STRIKE_RING_RADIUS = 380.0f;
    constexpr unsigned int STRIKE_RING_COLOR = 0x96DCFF;

    // 画面の手応え 止める秒数 揺れの強さと長さ 寄る角度と長さ 光らせる色と濃さと長さ
    constexpr float STRIKE_HIT_STOP = 0.15f;
    constexpr float STRIKE_SHAKE = 18.0f;
    constexpr float STRIKE_SHAKE_TIME = 0.6f;
    constexpr float STRIKE_ZOOM = 10.0f;
    constexpr float STRIKE_ZOOM_TIME = 0.6f;
    constexpr unsigned int STRIKE_FLASH_COLOR = 0xC8E8FF;
    constexpr float STRIKE_FLASH_ALPHA = 0.8f;
    constexpr float STRIKE_FLASH_TIME = 0.35f;

    COLOR_U8 ToColor(unsigned int rgb) {
        return GetColorU8((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255);
    }

    // 浮いている敵の下でも、輪は地面に広げる
    VECTOR ToGround(VECTOR position) {
        VECTOR ground = position;
        StageBuilder::FindGroundHeight(position.x, position.z, ground.y);
        return ground;
    }
}

void SpecialEffectObserver::OnNotify(const SpecialEvent& event) {
    switch (event.type) {
    case SpecialEvent::Type::Ready:  PlayReady(event);  break;
    case SpecialEvent::Type::Call:   PlayCall(event);   break;
    case SpecialEvent::Type::Strike: PlayStrike(event); break;
    }
}

void SpecialEffectObserver::PlayReady(const SpecialEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    effects->PlayShockwave(event.position, READY_RING_RADIUS, ToColor(READY_COLOR));
    effects->FlashScreen(READY_COLOR, READY_FLASH_ALPHA, READY_FLASH_TIME);
}

void SpecialEffectObserver::PlayCall(const SpecialEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    effects->FlashScreen(CALL_SHADE_COLOR, CALL_SHADE_ALPHA, event.callTime);
    effects->ZoomPunch(CALL_ZOOM, event.callTime);
    effects->PlayShockwave(event.position, CALL_RING_RADIUS, ToColor(CALL_RING_COLOR));
}

void SpecialEffectObserver::PlayStrike(const SpecialEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    // 敵ごとに雷を落とす
    for (const VECTOR& target : event.targets) {
        EffectManager::SpriteDesc bolt;
        bolt.position = target;
        bolt.width = LIGHTNING_WIDTH;
        bolt.height = LIGHTNING_HEIGHT;
        bolt.life = LIGHTNING_LIFE;
        bolt.fadeStart = LIGHTNING_FADE_START;
        bolt.flicker = LIGHTNING_FLICKER;
        bolt.color = ToColor(LIGHTNING_COLOR);
        effects->PlayLightning(bolt);

        effects->PlayShockwave(ToGround(target), STRIKE_RING_RADIUS, ToColor(STRIKE_RING_COLOR));
    }

    effects->HitStop(STRIKE_HIT_STOP);
    effects->Shake(STRIKE_SHAKE, STRIKE_SHAKE_TIME);
    effects->ZoomPunch(STRIKE_ZOOM, STRIKE_ZOOM_TIME);
    effects->FlashScreen(STRIKE_FLASH_COLOR, STRIKE_FLASH_ALPHA, STRIKE_FLASH_TIME);
}
