#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "JustDodgeEvent.h"

// ジャスト回避の知らせを受けて、見切った音を鳴らす
// 音は SoundManager のシングルトンから鳴らす
class JustDodgeSoundObserver : public MonoBehaviour, public Observer<JustDodgeEvent> {
public:
    void OnNotify(const JustDodgeEvent& event) override;
};
