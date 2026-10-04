#pragma once
#include "UIImage.h"

class Player;
class PhaseDirector;

// 戦闘中の表示
// フェーズ 残りの敵 次の全回復 コンボ 割られそうなときの画面の縁
// 体力はなく、吹っ飛ばされ値も数字では出さない 溜まり具合は体の赤みと湯気で見せる
class Hud : public UIImage {
private:
    Player* _player = nullptr;
    PhaseDirector* _director = nullptr;

    int _shownCombo = 0;

public:
    // 制作用 キャラの頭上に今の状態と吹っ飛ばされ値を出す
    bool isStateVisible = false;

    void Setup(Player* player, PhaseDirector* director);

    void Render() override;

private:
    void DrawDanger(int screenWidth, int screenHeight);
    void DrawPhaseInfo(int screenWidth, int screenHeight);
    void DrawCombo(int screenWidth, int screenHeight);
    void DrawControls(int screenWidth, int screenHeight);
    void DrawStates();
};
