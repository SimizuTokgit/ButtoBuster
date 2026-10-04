#include "ChargeSoundObserver.h"
#include "SoundManager.h"

void ChargeSoundObserver::OnNotify(const PlayerChargeEvent& event) {
    auto& sound = SoundManager::Instance();

    switch (event.type) {
    case PlayerChargeEvent::Type::LevelUp:
        // 溜めきった音だけ変え、画面を見ていなくても最大まで溜まったと分かるようにする
        if (event.level >= event.maxLevel) {
            sound.PlaySE("Player/guard_success");
        }
        else {
            sound.PlaySE("Common/system_counter", 0.8f);
        }
        break;

    // 溜め始めは、押してすぐ離したヘビーアタックでも毎回通るので鳴らさない
    // 振る音はアニメに付けてあるので、離したときとやめたときも鳴らさない
    case PlayerChargeEvent::Type::Start:
    case PlayerChargeEvent::Type::Release:
    case PlayerChargeEvent::Type::Cancel:
        break;
    }
}
