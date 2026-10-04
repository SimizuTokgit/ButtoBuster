#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "ChainEvent.h"

// 連鎖の知らせを受けて画面を揺らし、区切りの数で止めたり光らせたりする
// 巻き込むたびに止めると流れが重くなるので、止めるのは区切りの数に届いたときだけにする
class ChainEffectObserver : public MonoBehaviour, public Observer<ChainEvent> {
public:
    void OnNotify(const ChainEvent& event) override;
};
