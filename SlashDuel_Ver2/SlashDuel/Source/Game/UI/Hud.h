#pragma once
#include "UIImage.h"

class Player;
class PhaseDirector;

// 戦闘中の表示
// 体力 フェーズ 残りの敵 次の全回復 コンボ 敵の頭上の体力
class Hud : public UIImage {
private:
    // 減った体力を遅れて追いかける白いバーの速さ 1 秒で減らせる割合
    static constexpr float GHOST_SPEED = 0.6f;

    Player* _player = nullptr;
    PhaseDirector* _director = nullptr;

    float _ghostRatio = 1.0f;
    int _shownCombo = 0;

public:
    // 制作用 キャラの頭上に今の状態を出す
    bool isStateVisible = false;

    void Setup(Player* player, PhaseDirector* director);

    void Update(float deltaTime) override;
    void Render() override;

private:
    void DrawPlayerHp(int screenWidth, int screenHeight);
    void DrawPhaseInfo(int screenWidth, int screenHeight);
    void DrawCombo(int screenWidth, int screenHeight);
    void DrawEnemyBars();
    void DrawControls(int screenWidth, int screenHeight);
    void DrawStates();
};
