#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "SpecialEvent.h"

// 必殺技の知らせを受けて、画面の演出を出す
// ゲージが満タンになったら光らせ、雷を呼ぶ間は暗くして寄り、落ちた瞬間に敵ごとに雷の画像を出して止めて揺らす
//
// 演出を変えるときはこのクラスだけ書き換えればよい 数値は .cpp の先頭に並べてある
// 雷が当たるかどうかや強さは、ここではなく PlayerAttacks の GetSpecial で決まる
class SpecialEffectObserver : public MonoBehaviour, public Observer<SpecialEvent> {
public:
    void OnNotify(const SpecialEvent& event) override;

private:
    void PlayReady(const SpecialEvent& event);
    void PlayCall(const SpecialEvent& event);
    void PlayStrike(const SpecialEvent& event);
};
