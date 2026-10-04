#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "JustDodgeEvent.h"

// ジャスト回避の知らせを受けて、全体をゆっくりにし、画面を光らせる
// ゆっくりになった間に、反撃を狙って入れてもらう
class JustDodgeEffectObserver : public MonoBehaviour, public Observer<JustDodgeEvent> {
public:
    void OnNotify(const JustDodgeEvent& event) override;
};
