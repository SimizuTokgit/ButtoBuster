#pragma once
#include "MonoBehaviour.h"
#include "EnemyData.h"

class Player;
class PhaseDirector;
class Hud;
class CameraFollow;

// 制作用のチート
// F1 で一覧を出し、F3 から F12 と数字の 1 から 5 で使う
// ゲーム本体のコードにデバッグの分岐を混ぜないよう、切り替えの反映はここでまとめて行う
class DebugCheats : public MonoBehaviour {
private:
    Player* _player = nullptr;
    PhaseDirector* _director = nullptr;
    Hud* _hud = nullptr;
    CameraFollow* _camera = nullptr;

    bool _wasSlow = false;

    // 状態の作り直しを 1 秒ごとに数える (StatePool) F1 の一覧の下に出す
    float _poolTimer = 0.0f;
    long long _lastCreatedCount = 0;
    long long _lastHeapCount = 0;
    long long _createdPerSecond = 0;
    long long _heapPerSecond = 0;

    // 撮影 賢さだけを変えたグレゴブリンを、毎回同じ並びで出す 出した敵はこの数値を指したまま動くので、ここに持っておく
    EnemyData _filmingGoblin;
    int _filmingIntelligence = 0;

public:
    void Setup(Player* player, PhaseDirector* director, Hud* hud, CameraFollow* camera);
    void Update(float deltaTime) override;

private:
    // フェーズを止め、プレイヤーを最初の場所へ戻して、賢さ _filmingIntelligence のグレゴブリンを前に並べる
    void StartFilming();
};
