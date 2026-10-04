#pragma once
#include "UIImage.h"

class Player;
class PhaseDirector;

// 倒れたあとの結果
// 到達フェーズがスコア 最高記録を超えたら知らせる
class ResultScreen : public UIImage {
private:
    // 結果が出てから入力を受け付けるまで 連打でそのまま飛ばさないように
    static constexpr float INPUT_DELAY = 0.8f;

    Player* _player = nullptr;
    PhaseDirector* _director = nullptr;
    float _shownTime = 0.0f;
    bool _isRequested = false;

public:
    void Setup(Player* player, PhaseDirector* director);

    void Update(float deltaTime) override;
    void Render() override;
};
