#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "ChainEvent.h"
#include "WallBreakEvent.h"

class Player;

// 連鎖と壁割りの知らせを受けて、必殺技のゲージを溜める
// 当てた数で溜まる分は Player の AddCombo で足す 溜まる量は PlayerData の specialGain で始まる値
//
// 連鎖や壁割りを起こす側 (BlowChain と ArenaWall) は、ゲージがあることを知らない
// 溜まる決まりを変えるときは、このクラスだけ書き換えればよい
class SpecialGaugeObserver : public MonoBehaviour,
    public Observer<ChainEvent>,
    public Observer<WallBreakEvent> {
private:
    Player* _player = nullptr;

public:
    void Setup(Player* player) { _player = player; }

    // 連鎖で 1 体巻き込むごとに溜める
    void OnNotify(const ChainEvent& event) override;

    // 敵が壁を割ったら溜める プレイヤーが割られたときは溜めない
    void OnNotify(const WallBreakEvent& event) override;
};
