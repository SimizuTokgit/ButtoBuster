#include "JustDodgeEffectObserver.h"
#include "EffectManager.h"

namespace {
    // ジャスト回避が決まった瞬間の演出
    struct JustDodgeLook {
        // 全体を止める秒数 プレイヤーは寄って反撃するので、長くは止めない
        float stopTime = 0.4f;

        // かわしたと分かるよう画面を光らせる 色 (0xRRGGBB) と濃さと長さ
        // 色は弧や輪の水色 (JUST_DODGE_COLOR) より少し濃い水色
        unsigned int flashColor = 0x60D0FF;
        float flashAlpha = 0.3f;
        float flashTime = 0.25f;

        // 寄る角度と、寄っている長さ 秒
        float zoomDegree = 6.0f;
        float zoomTime = 0.5f;

        // 足元に広げる輪の大きさ 色は JUST_DODGE_COLOR
        float ringRadius = 260.0f;
    };
    constexpr JustDodgeLook LOOK{};
}

void JustDodgeEffectObserver::OnNotify(const JustDodgeEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    effects->HitStop(LOOK.stopTime);
    effects->FlashScreen(LOOK.flashColor, LOOK.flashAlpha, LOOK.flashTime);

    // 見切った瞬間を大きく見せる
    effects->ZoomPunch(LOOK.zoomDegree, LOOK.zoomTime);
    effects->PlayShockwave(event.position, LOOK.ringRadius, EffectManager::ToColor(JUST_DODGE_COLOR));
}
