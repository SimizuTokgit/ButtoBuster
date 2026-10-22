#include "WallBreakSoundObserver.h"
#include "SoundManager.h"

namespace {
    // 重い一撃の音を 2 つ重ねて、壁が砕けた手応えにする
    const char* const CRASH_SOUND = "Golem/attack_stomp";
    const char* const IMPACT_SOUND = "Player/dmg_byRockKnuckle";
    constexpr float IMPACT_SOUND_VOLUME = 0.9f;

    // プレイヤーが割られたときは、もう 1 つ重ねて重くする
    const char* const PLAYER_EXTRA_SOUND = "Golem/downing";
}

void WallBreakSoundObserver::OnNotify(const WallBreakEvent& event) {
    auto& sound = SoundManager::Instance();

    sound.PlaySE(CRASH_SOUND);
    sound.PlaySE(IMPACT_SOUND, IMPACT_SOUND_VOLUME);
    if (event.isPlayer) sound.PlaySE(PLAYER_EXTRA_SOUND);
}
