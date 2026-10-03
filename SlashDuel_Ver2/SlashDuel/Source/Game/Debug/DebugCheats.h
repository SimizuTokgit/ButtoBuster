#pragma once
#include "MonoBehaviour.h"

class Player;
class PhaseDirector;
class Hud;

// 制作用のチート
// F1 で一覧を出し、F3 から F11 で使う
// ゲーム本体のコードにデバッグの分岐を混ぜないよう、切り替えの反映はここでまとめて行う
class DebugCheats : public MonoBehaviour {
private:
    Player* _player = nullptr;
    PhaseDirector* _director = nullptr;
    Hud* _hud = nullptr;

    bool _wasSlow = false;

public:
    void Setup(Player* player, PhaseDirector* director, Hud* hud);
    void Update(float deltaTime) override;
};
