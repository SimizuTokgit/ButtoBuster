#pragma once
#include "UIImage.h"
#include "Observer.h"
#include "JustDodgeEvent.h"
#include "DxLib.h"

// ジャスト回避が決まったとき、かわした場所の頭の上に文字を出す
// 知らせを受けて出すだけで、どうかわしたかは知らない
class JustDodgeBanner : public UIImage, public Observer<JustDodgeEvent> {
private:
    // 出している時間と、そのうち最後に消えていく時間 実時間の秒
    static constexpr float SHOW_TIME = 0.9f;
    static constexpr float FADE_TIME = 0.3f;

    // 出た瞬間だけ大きな文字にする時間
    static constexpr float POP_TIME = 0.12f;

    // 出ている間に浮き上がる高さ 画面のドット
    static constexpr float RISE_HEIGHT = 30.0f;

    VECTOR _worldPosition = VGet(0.0f, 0.0f, 0.0f);

    // 残りの表示時間 0 なら出していない
    float _timer = 0.0f;

public:
    void Setup();

    void OnNotify(const JustDodgeEvent& event) override;
    void Update(float deltaTime) override;
    void Render() override;
};
