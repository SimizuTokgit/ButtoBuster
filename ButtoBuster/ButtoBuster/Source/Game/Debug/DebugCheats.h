#pragma once
#include "MonoBehaviour.h"

class Player;
class PhaseDirector;
class Hud;

// 制作用のチート
// F1 で一覧を出し、F3 から F12 で使う
// ゲーム本体のコードにデバッグの分岐を混ぜないよう、切り替えの反映はここでまとめて行う
class DebugCheats : public MonoBehaviour {
private:
    Player* _player = nullptr;
    PhaseDirector* _director = nullptr;
    Hud* _hud = nullptr;

    bool _wasSlow = false;

    // 状態の作り直しを 1 秒ごとに数える (StatePool) F1 の一覧の下に出す
    float _poolTimer = 0.0f;
    long long _lastCreatedCount = 0;
    long long _lastHeapCount = 0;
    long long _createdPerSecond = 0;
    long long _heapPerSecond = 0;

public:
    void Setup(Player* player, PhaseDirector* director, Hud* hud);
    void Update(float deltaTime) override;
};
