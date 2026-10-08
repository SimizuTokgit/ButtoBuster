#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "WallBreakEvent.h"

// 壁割りの知らせを受けて、壁が砕けた音を鳴らす
// 音を変えるときはこのクラスだけ書き換えればよい 鳴らす音の名前は .cpp の先頭に並べてある
// 倒された声は Enemy と Player の Defeat が、負けたときのジングルは PhaseDirector が鳴らす
class WallBreakSoundObserver : public MonoBehaviour, public Observer<WallBreakEvent> {
public:
    void OnNotify(const WallBreakEvent& event) override;
};
