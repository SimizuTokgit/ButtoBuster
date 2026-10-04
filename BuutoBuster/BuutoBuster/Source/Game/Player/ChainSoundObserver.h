#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "ChainEvent.h"

// 連鎖の知らせを受けて、体どうしがぶつかった音を鳴らす
// 音は SoundManager のシングルトンから鳴らすので、続けて当たっても同じ音の重なりは間引かれる
class ChainSoundObserver : public MonoBehaviour, public Observer<ChainEvent> {
public:
    void OnNotify(const ChainEvent& event) override;
};
