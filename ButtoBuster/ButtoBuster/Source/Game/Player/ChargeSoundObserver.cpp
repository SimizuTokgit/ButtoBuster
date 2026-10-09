#include "ChargeSoundObserver.h"
#include "SoundManager.h"

void ChargeSoundObserver::OnNotify(const PlayerChargeEvent& event) {
    auto& sound = SoundManager::Instance();

    switch (event.type) {
    case PlayerChargeEvent::Type::LevelUp: {
        // 段ごとに音を変え、画面を見ていなくてもどこまで溜まったか分かるようにする
        static const char* const LEVEL_SOUNDS[] = { "Player/SE_Charge1", "Player/SE_Charge2", "Player/SE_Charge3" };
        constexpr int LEVEL_SOUND_COUNT = sizeof(LEVEL_SOUNDS) / sizeof(LEVEL_SOUNDS[0]);
        int index = event.level - 1;
        if (index >= LEVEL_SOUND_COUNT) index = LEVEL_SOUND_COUNT - 1;
        if (index >= 0) sound.PlaySE(LEVEL_SOUNDS[index]);

        // 溜めの声は 1 段目に届いたときに出す 押してすぐ離したヘビーアタックでは鳴らないように
        if (event.level == 1) sound.PlaySE("Player/Vc_Charge", 0.8f);
        break;
    }

    // 溜め始めは、押してすぐ離したヘビーアタックでも毎回通るので鳴らさない
    // 振る音はアニメに、離したときの声は PlayerAttackState にあるので、離したときとやめたときも鳴らさない
    case PlayerChargeEvent::Type::Start:
    case PlayerChargeEvent::Type::Release:
    case PlayerChargeEvent::Type::Cancel:
        break;
    }
}
