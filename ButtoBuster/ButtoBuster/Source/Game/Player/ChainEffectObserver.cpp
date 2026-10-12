#include "ChainEffectObserver.h"
#include "EffectManager.h"

namespace {
    // 巻き込むたびの揺れ 連鎖が伸びるほど強くし、強くなりすぎないよう上限を付ける
    constexpr float SHAKE_BASE = 2.0f;
    constexpr float SHAKE_PER_COUNT = 0.3f;
    constexpr float SHAKE_MAX = 7.0f;
    constexpr float SHAKE_TIME = 0.12f;

    // 区切りの数に届いたときの演出 使わないものは 0
    struct MilestoneLook {
        // どの区切りか (ChainMilestone)
        int count;

        // 止める秒数
        float hitStop;

        // ゆっくりにする速さ (1 で普通) と長さ
        float slowScale;
        float slowTime;

        // 一瞬寄る角度 度 と長さ
        float zoom;
        float zoomTime;

        // 画面の揺れの強さと長さ
        float shake;
        float shakeTime;

        // 画面を光らせる色 (0xRRGGBB) と濃さと長さ
        unsigned int flashColor;
        float flashAlpha;
        float flashTime;
    };

    const MilestoneLook MILESTONE_LOOKS[] = {
        // 1 つ目 少し止めて光らせる
        {
            ChainMilestone::FIRST,
            0.05f,                      // hitStop
            0.0f, 0.0f,                 // slowScale slowTime
            0.0f, 0.0f,                 // zoom zoomTime
            0.0f, 0.0f,                 // shake shakeTime
            0xFFE0A0, 0.12f, 0.12f,     // flashColor flashAlpha flashTime
        },
        // 2 つ目 一瞬止めてからゆっくり流し、群れが飛んでいく様子を見せる
        {
            ChainMilestone::SECOND,
            0.08f,                      // hitStop
            0.35f, 0.5f,                // slowScale slowTime
            4.0f, 0.3f,                 // zoom zoomTime
            0.0f, 0.0f,                 // shake shakeTime
            0xFFFFFF, 0.25f, 0.2f,      // flashColor flashAlpha flashTime
        },
        // 3 つ目 大きく揺らす
        {
            ChainMilestone::THIRD,
            0.1f,                       // hitStop
            0.0f, 0.0f,                 // slowScale slowTime
            0.0f, 0.0f,                 // zoom zoomTime
            12.0f, 0.35f,               // shake shakeTime
            0xFFD040, 0.3f, 0.3f,       // flashColor flashAlpha flashTime
        },
    };
}

void ChainEffectObserver::OnNotify(const ChainEvent& event) {
    if (event.type != ChainEvent::Type::Hit) return;

    auto* effects = EffectManager::Get();
    if (!effects) return;

    float power = SHAKE_BASE + SHAKE_PER_COUNT * event.count;
    if (power > SHAKE_MAX) power = SHAKE_MAX;
    effects->Shake(power, SHAKE_TIME);

    for (const MilestoneLook& look : MILESTONE_LOOKS) {
        if (event.count != look.count) continue;

        effects->HitStop(look.hitStop);
        if (look.slowTime > 0.0f) effects->SlowMotion(look.slowScale, look.slowTime);
        if (look.zoom > 0.0f) effects->ZoomPunch(look.zoom, look.zoomTime);
        if (look.shake > 0.0f) effects->Shake(look.shake, look.shakeTime);
        effects->FlashScreen(look.flashColor, look.flashAlpha, look.flashTime);
    }
}
