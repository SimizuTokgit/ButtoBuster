#include "SpecialSoundObserver.h"
#include "SoundManager.h"

namespace {
    // 雷の音はまだ無いので、近い音を借りている 雷の音を Data/Sound/SE に入れたら名前を書き換える
    // 空なら鳴らさない

    // ゲージが満タンになったとき
    constexpr const char* READY_SOUND = "Common/system_counter";
    constexpr float READY_VOLUME = 0.8f;

    // 剣を掲げて雷を呼んだとき
    constexpr const char* CALL_SOUND = "Golem/attack_stompPre";
    constexpr float CALL_VOLUME = 1.0f;

    // 雷が落ちたとき 2 つ重ねて鳴らす
    constexpr const char* STRIKE_SOUND = "Golem/attack_stomp";
    constexpr const char* STRIKE_SOUND_2 = "Player/guard_success";
    constexpr float STRIKE_VOLUME = 1.0f;

    void Play(const char* name, float volume) {
        if (name[0] == '\0') return;
        SoundManager::Instance().PlaySE(name, volume);
    }
}

void SpecialSoundObserver::OnNotify(const SpecialEvent& event) {
    switch (event.type) {
    case SpecialEvent::Type::Ready:
        Play(READY_SOUND, READY_VOLUME);
        break;

    case SpecialEvent::Type::Call:
        Play(CALL_SOUND, CALL_VOLUME);
        break;

    case SpecialEvent::Type::Strike:
        Play(STRIKE_SOUND, STRIKE_VOLUME);
        Play(STRIKE_SOUND_2, STRIKE_VOLUME);
        break;
    }
}
