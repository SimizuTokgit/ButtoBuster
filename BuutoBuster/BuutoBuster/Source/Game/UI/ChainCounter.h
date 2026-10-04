#pragma once
#include "UIImage.h"
#include "Observer.h"
#include "ChainEvent.h"

// 連鎖ぶっ飛ばしで巻き込んだ数を、画面の上に大きく出す
// 連鎖の知らせを受けて数を変えるだけで、連鎖がどう起きたかは知らない
class ChainCounter : public UIImage, public Observer<ChainEvent> {
private:
    // これより少ないときは出さない 1 体巻き込んだだけで毎回出るとうるさい
    static constexpr int MIN_SHOWN_COUNT = 2;

    // 連鎖が止まってから消え始めるまでと、消えきるまで 秒
    static constexpr float HOLD_TIME = 1.2f;
    static constexpr float FADE_TIME = 0.4f;

    // 止まった知らせが来なくても、これだけ何も起きなければ止まったとみなす
    // 飛んでいる途中で地形の穴に落ちて片付けられた敵がいると、止まった知らせが来ないため
    static constexpr float QUIET_TIME = 2.0f;

    // 巻き込むたびに数字が跳ねる高さと時間
    static constexpr float BOUNCE_HEIGHT = 14.0f;
    static constexpr float BOUNCE_TIME = 0.15f;

    int _count = 0;
    bool _hasEnded = false;

    // 最後に知らせが来てからの時間
    float _timer = 0.0f;
    float _bounceTimer = 0.0f;

public:
    void Setup();

    void OnNotify(const ChainEvent& event) override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    float GetAlpha() const;
};
