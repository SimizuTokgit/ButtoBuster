#pragma once
#include "ICharacterState.h"
#include "AttackData.h"

class Player;

// 空中の△ 真下へ叩きつける
// 宙で一瞬振りかぶってから一気に落ち、着地した場所から衝撃波を広げて周りの敵を吹き飛ばす
// 吹き飛ばした敵は連鎖の砲弾になり、群れの外へも広がっていく
class PlayerAirSlamState : public ICharacterState<Player> {
private:
    // 振りかぶりで止める姿勢 アニメのフレーム ヘビーアタックの溜めと同じ、振り下ろす直前
    static constexpr float POSE_TIME = 5.5f;

    // 振り抜き終わるフレーム 刃の軌跡をここまで出す
    static constexpr float SWING_END_TIME = 13.0f;

    // 地面に着かないまま落ち続けたときに、叩きつけたことにするまでの時間 秒
    // 落ちる速さと着地の隙は PlayerParams
    static constexpr float DIVE_TIME_LIMIT = 1.5f;

    enum class Phase {
        Windup,     // 宙で止まって振りかぶる
        Dive,       // 真下へ落ちる
        Land,       // 叩きつけて振り抜く
    };

    // 強化の倍率を掛けたり、ジャスト回避のあとなら反撃に書き換えたりするので、技の数値は写しを持つ
    AttackData _data;

    Phase _phase = Phase::Windup;
    float _timer = 0.0f;

    // 着地の隙の間に押された次の技 隙が明けたら出す
    Technique _queued = Technique::None;

public:
    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override { return "AirSlam"; }

private:
    void StartDive(Player& player);
    void Land(Player& player);
};
