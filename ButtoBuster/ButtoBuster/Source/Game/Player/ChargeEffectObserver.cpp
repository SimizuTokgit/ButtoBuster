#include "ChargeEffectObserver.h"
#include "EffectManager.h"
#include "PlayerAttacks.h"

namespace {
    // 段階ごとの輪の大きさ 上がるほど大きくする
    // 色は、その段階で振ったときの斬撃の弧の色 (PlayerAttacks::GetHeavy) 金から赤へ寄っていく
    constexpr float RING_RADIUS_BASE = 120.0f;
    constexpr float RING_RADIUS_PER_LEVEL = 80.0f;

    // 溜めきったときの画面の手応え
    struct MaxLook {
        // 画面を光らせる色 (0xRRGGBB) と濃さと長さ
        unsigned int flashColor = 0xFFC060;
        float flashAlpha = 0.15f;
        float flashTime = 0.15f;

        // 画面の揺れの強さと長さ
        float shake = 3.0f;
        float shakeTime = 0.15f;
    };
    constexpr MaxLook MAX_LOOK{};
}

void ChargeEffectObserver::OnNotify(const PlayerChargeEvent& event) {
    if (event.type != PlayerChargeEvent::Type::LevelUp) return;

    auto* effects = EffectManager::Get();
    if (!effects) return;

    bool isMax = event.level >= event.maxLevel;

    effects->PlayWarning(event.headPosition, isMax);

    float radius = RING_RADIUS_BASE + RING_RADIUS_PER_LEVEL * event.level;
    effects->PlayShockwave(event.position, radius, PlayerAttacks::GetHeavy(event.level).arcColor);

    if (isMax) {
        effects->FlashScreen(MAX_LOOK.flashColor, MAX_LOOK.flashAlpha, MAX_LOOK.flashTime);
        effects->Shake(MAX_LOOK.shake, MAX_LOOK.shakeTime);
    }
}
