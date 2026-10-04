#include "WallBreakEffectObserver.h"
#include "EffectManager.h"
#include "ArenaBoundary.h"
#include "StageBuilder.h"

namespace {
    // 演出の強さ 敵が割れたときと、プレイヤーが割られたとき (負け) で分ける
    struct BreakLook {
        // 止める秒数
        float hitStop;

        // ゆっくりにする速さ (1 で普通) と長さ 長さは実時間の秒 寄るのもこの長さ
        float slowScale;
        float slowTime;

        // 画面の揺れの強さと長さ
        float shake;
        float shakeTime;

        // 一瞬寄る角度 度
        float zoom;

        // 画面を光らせる色 (0xRRGGBB) と濃さと長さ
        unsigned int flashColor;
        float flashAlpha;
        float flashTime;

        // 割れた所の光の幕が消えるまでの秒数と、左右に広げる角度 度
        float crackTime;
        float crackSpread;

        // 壁の足元に広げる輪の大きさと色 (0xRRGGBB)
        float shockwaveRadius;
        unsigned int shockwaveColor;
    };

    // 敵が割れたとき 何度も起きるので、気持ちよく、でも戦いの流れは止めすぎない
    const BreakLook ENEMY_LOOK = {
        0.12f,              // hitStop
        0.35f, 0.5f,        // slowScale slowTime
        14.0f, 0.4f,        // shake shakeTime
        6.0f,               // zoom
        0xFFFFFF, 0.35f, 0.2f,  // flashColor flashAlpha flashTime
        1.2f, 18.0f,        // crackTime crackSpread
        600.0f, 0x96DCFF,   // shockwaveRadius shockwaveColor
    };

    // プレイヤーが割られたとき ゲームの終わりなので、長く止めて大きく見せる
    const BreakLook PLAYER_LOOK = {
        0.25f,              // hitStop
        0.15f, 1.5f,        // slowScale slowTime
        22.0f, 0.8f,        // shake shakeTime
        12.0f,              // zoom
        0xFF3030, 0.5f, 0.4f,   // flashColor flashAlpha flashTime
        2.0f, 26.0f,        // crackTime crackSpread
        900.0f, 0xFF5A50,   // shockwaveRadius shockwaveColor
    };

    COLOR_U8 ToColor(unsigned int rgb) {
        return GetColorU8((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255);
    }
}

void WallBreakEffectObserver::OnNotify(const WallBreakEvent& event) {
    auto* effects = EffectManager::Get();
    if (!effects) return;

    const BreakLook& look = event.isPlayer ? PLAYER_LOOK : ENEMY_LOOK;

    // 割れた壁のかけらを外へ散らす 体はこの向きへ飛んでいく
    effects->PlayWallBreak(event.position, VScale(event.normal, -1.0f));

    // 時間を止めてからゆっくり流し、割れた瞬間を見せる
    effects->HitStop(look.hitStop);
    effects->SlowMotion(look.slowScale, look.slowTime);
    effects->Shake(look.shake, look.shakeTime);
    effects->ZoomPunch(look.zoom, look.slowTime);
    effects->FlashScreen(look.flashColor, look.flashAlpha, look.flashTime);

    // 割れた所の光の幕を大きく長く光らせる
    if (auto* boundary = ArenaBoundary::Get()) {
        boundary->Flash(event.position, 1.0f, look.crackTime, look.crackSpread);
    }

    // 壁の足元に輪を広げる
    VECTOR ground = event.position;
    StageBuilder::FindGroundHeight(event.position.x, event.position.z, ground.y);
    effects->PlayShockwave(ground, look.shockwaveRadius, ToColor(look.shockwaveColor));
}
