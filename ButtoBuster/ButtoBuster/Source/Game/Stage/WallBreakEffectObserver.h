#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "WallBreakEvent.h"

// 壁割りの知らせを受けて、画面の演出を出す
// 時間を止めてからゆっくり流し、揺らして寄り、割れた壁のかけらと光とヒビの画像で割れた瞬間を見せる
//
// 演出を変えるときはこのクラスだけ書き換えればよい 壁を割る側 (ArenaWall) は触らなくてよい
// 強さの数値は .cpp の先頭に、敵が割れたときとプレイヤーが割られたときに分けて並べてある
class WallBreakEffectObserver : public MonoBehaviour, public Observer<WallBreakEvent> {
public:
    void OnNotify(const WallBreakEvent& event) override;
};
