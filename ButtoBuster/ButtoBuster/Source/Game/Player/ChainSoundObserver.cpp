#include "ChainSoundObserver.h"
#include "SoundManager.h"

namespace {
    // 巻き込んだときの鈍い音の大きさ
    constexpr float HIT_VOLUME = 0.8f;
}

void ChainSoundObserver::OnNotify(const ChainEvent& event) {
    if (event.type != ChainEvent::Type::Hit) return;

    auto& sound = SoundManager::Instance();

    // 斬った音ではなく、重いものがぶつかった鈍い音にする
    sound.PlaySE("Player/dmg_byRockKnuckle", HIT_VOLUME);

    // 大きな区切りに届いたら、地面を揺らすような重い音を足す
    if (event.count == ChainMilestone::SECOND || event.count == ChainMilestone::THIRD) {
        sound.PlaySE("Golem/attack_stomp");
    }
}
