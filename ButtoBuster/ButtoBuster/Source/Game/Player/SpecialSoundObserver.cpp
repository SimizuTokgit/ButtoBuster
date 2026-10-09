#include "SpecialSoundObserver.h"
#include "SoundManager.h"

namespace {
    // 空なら鳴らさない

    // ゲージが満タンになったとき
    constexpr const char* READY_SOUND = "Player/SE_Thunder_Ready";
    constexpr float READY_VOLUME = 1.0f;

    // 剣を掲げて雷を呼んだとき
    constexpr const char* CALL_SOUND = "Golem/attack_stompPre";
    constexpr float CALL_VOLUME = 1.0f;

    // 雷が落ちたとき 雷の音と、必殺技の声を重ねて鳴らす
    constexpr const char* STRIKE_SOUND = "Player/SE_Thunder";
    constexpr const char* STRIKE_SOUND_2 = "Player/Vc_Attack_Special";
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
