#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "JustDodgeEvent.h"

// ジャスト回避の知らせを受けて、一瞬止めて画面を光らせ、少し寄る
// 敵がゆっくりになるのは Player の反撃の決まり (PlayerData の counterSlowScale) で、ここでは見せ方だけを決める
class JustDodgeEffectObserver : public MonoBehaviour, public Observer<JustDodgeEvent> {
public:
    void OnNotify(const JustDodgeEvent& event) override;
};
