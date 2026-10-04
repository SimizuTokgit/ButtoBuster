#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "PlayerChargeEvent.h"

// 溜めの知らせを受けて音を鳴らす
// 音は SoundManager のシングルトンから鳴らすので、どこから鳴らしても音量の設定と重なりの間引きが同じように効く
class ChargeSoundObserver : public MonoBehaviour, public Observer<PlayerChargeEvent> {
public:
    void OnNotify(const PlayerChargeEvent& event) override;
};
