#include "ChainEffectObserver.h"
#include "EffectManager.h"

namespace {
    // 巻き込むたびの揺れ 連鎖が伸びるほど強くし、強くなりすぎないよう上限を付ける
    constexpr float SHAKE_BASE = 2.0f;
    constexpr float SHAKE_PER_COUNT = 0.3f;
    constexpr float SHAKE_MAX = 7.0f;
    constexpr float SHAKE_TIME = 0.12f;
}

void ChainEffectObserver::OnNotify(const ChainEvent& event) {
    if (event.type != ChainEvent::Type::Hit) return;

    auto* effects = EffectManager::Get();
    if (!effects) return;

    float power = SHAKE_BASE + SHAKE_PER_COUNT * event.count;
    if (power > SHAKE_MAX) power = SHAKE_MAX;
    effects->Shake(power, SHAKE_TIME);

    if (event.count == ChainMilestone::FIRST) {
        effects->HitStop(0.05f);
        effects->FlashScreen(0xFFE0A0, 0.12f, 0.12f);
    }
    else if (event.count == ChainMilestone::SECOND) {
        // 一瞬止めてからゆっくり流し、群れが飛んでいく様子を見せる
        effects->HitStop(0.08f);
        effects->SlowMotion(0.35f, 0.5f);
        effects->ZoomPunch(4.0f, 0.3f);
        effects->FlashScreen(0xFFFFFF, 0.25f, 0.2f);
    }
    else if (event.count == ChainMilestone::THIRD) {
        effects->HitStop(0.1f);
        effects->Shake(12.0f, 0.35f);
        effects->FlashScreen(0xFFD040, 0.3f, 0.3f);
    }
}
