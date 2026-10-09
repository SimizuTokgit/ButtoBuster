#pragma once
#include "UIImage.h"

class Player;
class PhaseDirector;

// 戦闘中の表示
// 回避の残り バースターゲージ (必殺技のゲージ) フェーズ 残りの敵 次の全回復 コンボ 割られそうなときの画面の縁
// 体力はなく、吹っ飛ばされ値も数字では出さない 溜まり具合は体の赤みと湯気で見せる
// 回避ゲージとバースターゲージの場所と大きさは .cpp の先頭に並べてある
class Hud : public UIImage {
private:
    Player* _player = nullptr;
    PhaseDirector* _director = nullptr;

    int _shownCombo = 0;

    // 危なさの音を最後に鳴らした脈の番号 同じ脈で 2 回鳴らさないように
    int _lastDangerBeat = -1;

    // 回避ゲージの画像 枠 (左の紋章つき) と、1 回分の中身
    int _dodgeFrameGraph = -1;
    int _dodgeFillGraph = -1;

    // バースターゲージの画像 枠 中身 満タンで出る BUSTER!! の文字
    int _specialFrameGraph = -1;
    int _specialFillGraph = -1;
    int _specialLogoGraph = -1;

    // 満タンになった時刻 (ミリ秒) 文字が現れる動きはここから数える
    bool _wasSpecialReady = false;
    int _specialReadyTime = 0;

public:
    // 制作用 キャラの頭上に今の状態と吹っ飛ばされ値、隙があればその残りを出す
    bool isStateVisible = false;

    ~Hud() override;

    void Setup(Player* player, PhaseDirector* director);

    void Render() override;

private:
    void DrawDanger(int screenWidth, int screenHeight);
    void DrawDodgeStock();
    void DrawSpecialGauge();
    void DrawPhaseInfo(int screenWidth, int screenHeight);
    void DrawCombo(int screenWidth, int screenHeight);
    void DrawControls(int screenWidth, int screenHeight);
    void DrawStates();
};
