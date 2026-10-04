#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "PlayerChargeEvent.h"

// 溜めの知らせを受けて光らせる
// 段階が上がるたびに頭の上と足元を光らせ、溜めきったら画面も光らせて揺らす
class ChargeEffectObserver : public MonoBehaviour, public Observer<PlayerChargeEvent> {
public:
    void OnNotify(const PlayerChargeEvent& event) override;
};
