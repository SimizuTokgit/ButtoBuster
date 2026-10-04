#include "ChainSoundObserver.h"
#include "SoundManager.h"

void ChainSoundObserver::OnNotify(const ChainEvent& event) {
    if (event.type != ChainEvent::Type::Hit) return;

    auto& sound = SoundManager::Instance();

    // 斬った音ではなく、重いものがぶつかった鈍い音にする
    sound.PlaySE("Player/dmg_byRockKnuckle", 0.8f);

    // 大きな区切りに届いたら、地面を揺らすような重い音を足す
    if (event.count == ChainMilestone::SECOND || event.count == ChainMilestone::THIRD) {
        sound.PlaySE("Golem/attack_stomp");
    }
}
