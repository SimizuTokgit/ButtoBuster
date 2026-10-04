#pragma once
#include "UIImage.h"

class Player;

// 溜めで止める姿勢を、遊びながら探すための制作用の機能
// P で止めるアニメを替え、, と . で止める時間を動かす 溜めている間なら、その場で姿勢が変わる
// 一度使うと、今の姿勢を画面の左に出し続ける
// 見つけた値を PlayerChargeState.h に書けば、普段の溜めの姿勢になる
class ChargePoseDebugger : public UIImage {
private:
    // 1 回押したときに動かす時間 アニメのフレーム
    static constexpr float FRAME_STEP = 0.5f;

    // 押し続けたとき、続けて動き始めるまでの時間と、そのあと動かす間隔 秒
    static constexpr float REPEAT_DELAY = 0.35f;
    static constexpr float REPEAT_INTERVAL = 0.08f;

    Player* _player = nullptr;

    bool _isActive = false;
    float _repeatTimer = 0.0f;

public:
    void Setup(Player* player);

    void Update(float deltaTime) override;
    void Render() override;

private:
    void ChangeAnimation(int direction);
    void StepTime(float amount);
    void UpdateStep();
    float GetTotalTime() const;
};
