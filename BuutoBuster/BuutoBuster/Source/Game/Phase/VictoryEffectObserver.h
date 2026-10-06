#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "VictoryEvent.h"

// 勝ちの知らせを受けて、画面の演出を出す
// VICTORY の文字が出る瞬間に、画面を光らせて揺らし、少し寄って、プレイヤーの足元に金の輪を広げる
//
// 演出を変えるときはこのクラスだけ書き換えればよい 勝ちを決める側 (PhaseDirector) は触らなくてよい
// 強さの数値は .cpp の先頭に並べてある
// 別の演出を足したいときは、Observer<VictoryEvent> を継承したクラスを作り、GameScene.cpp で登録する
class VictoryEffectObserver : public MonoBehaviour, public Observer<VictoryEvent> {
public:
    void OnNotify(const VictoryEvent& event) override;
};
