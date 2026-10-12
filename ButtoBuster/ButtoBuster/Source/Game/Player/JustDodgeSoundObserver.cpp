#include "JustDodgeSoundObserver.h"
#include "SoundManager.h"

namespace {
    // 刃がすぐ横をすり抜けた音の大きさ
    constexpr float SWISH_VOLUME = 0.8f;
}

void JustDodgeSoundObserver::OnNotify(const JustDodgeEvent& event) {
    auto& sound = SoundManager::Instance();

    // 攻撃を弾いたときの澄んだ音に、刃がすぐ横をすり抜けた音を重ねる
    sound.PlaySE("Player/guard_success");
    sound.PlaySE("Weapon/Sabel/swish_L2", SWISH_VOLUME);
    sound.PlaySE("Player/Vc_Dodge_Jast");
}
