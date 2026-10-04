#include "ChargeEffectObserver.h"
#include "EffectManager.h"

namespace {
    // 段階ごとの輪の大きさと色 上がるほど大きく、金から赤へ寄せる
    // 振ったときの斬撃の弧の色とそろえてある
    constexpr float RING_RADIUS_BASE = 120.0f;
    constexpr float RING_RADIUS_PER_LEVEL = 80.0f;

    COLOR_U8 GetLevelColor(int level) {
        if (level <= 1) return GetColorU8(255, 215, 130, 255);
        if (level == 2) return GetColorU8(255, 185, 90, 255);
        return GetColorU8(255, 130, 70, 255);
    }
}

void ChargeEffectObserver::OnNotify(const PlayerChargeEvent& event) {
    if (event.type != PlayerChargeEvent::Type::LevelUp) return;

    auto* effects = EffectManager::Get();
    if (!effects) return;

    bool isMax = event.level >= event.maxLevel;

    effects->PlayWarning(event.headPosition, isMax);

    float radius = RING_RADIUS_BASE + RING_RADIUS_PER_LEVEL * event.level;
    effects->PlayShockwave(event.position, radius, GetLevelColor(event.level));

    if (isMax) {
        effects->FlashScreen(0xFFC060, 0.15f, 0.15f);
        effects->Shake(3.0f, 0.15f);
    }
}
