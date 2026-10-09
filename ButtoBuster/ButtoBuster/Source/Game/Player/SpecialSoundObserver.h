#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "SpecialEvent.h"

// 必殺技の知らせを受けて、音を鳴らす
// 鳴らす音の名前は .cpp の先頭に並べてある 剣を振り抜く音と声は、Attack3 のアニメに付いている音が鳴る
class SpecialSoundObserver : public MonoBehaviour, public Observer<SpecialEvent> {
public:
    void OnNotify(const SpecialEvent& event) override;
};
