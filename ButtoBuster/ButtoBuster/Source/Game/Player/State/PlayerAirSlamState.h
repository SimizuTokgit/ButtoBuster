#pragma once
#include "ICharacterState.h"
#include "AttackData.h"
#include "PlayerActions.h"
#include <vector>

class Player;
class Character;

// 空中の強攻撃 (攻撃 + ガード) 真下へ叩きつける
// 宙で一瞬振りかぶってから一気に落ちる 落ちる途中で宙の敵を捕まえ、一緒に地面へ叩き落とす (敵は地面で跳ね上がる)
// 着地したら、前の扇形 (溜め斬りと同じ範囲) の敵を吹き飛ばす
// 吹き飛ばした敵は連鎖の砲弾になり、群れの外へも広がっていく
// 振りかぶりで止めるフレームと、振り抜き終わるフレームはヘビーアタックの溜めと同じ (PlayerData の chargePoseTime と chargeSwingEndTime)
class PlayerAirSlamState : public ICharacterState<Player> {
private:
    // 地面に着かないまま落ち続けたときに、叩きつけたことにするまでの時間 秒
    // 落ちる速さと着地の隙は PlayerData
    static constexpr float DIVE_TIME_LIMIT = 1.5f;

    // 叩きつけたときに画面を揺らす長さ 秒 強さは技の shake 外しても揺らす
    static constexpr float LAND_SHAKE_TIME = 0.3f;

    enum class Phase {
        Windup,     // 宙で止まって振りかぶる
        Dive,       // 真下へ落ちる
        Land,       // 叩きつけて振り抜く
    };

    // 強化の倍率を掛けたり、ジャスト回避のあとなら反撃に書き換えたりするので、技の数値は写しを持つ
    AttackData _data;

    // 着地してから動けるまで 秒 OP 相殺で伸びた分も入れてある
    float _recoveryTime = 0.0f;

    // OP 相殺で数えた回数 捕まえる当たりにも、同じだけ掛ける
    int _staleCount = 0;

    // 落ちる途中で捕まえた敵 同じ敵を 2 回捕まえず、着地の吹き飛ばしでも跳ね上がりを邪魔しないように覚えておく
    std::vector<Character*> _caught;

    Phase _phase = Phase::Windup;
    float _timer = 0.0f;

    // 着地の隙の間に押された次の技 隙が明けたら出す
    PlayerActions::QueuedTechnique _queued;

public:
    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override { return "AirSlam"; }
    float GetOpeningTime(const Player& player) const override;

private:
    void StartDive(Player& player);
    void Catch(Player& player);
    void Land(Player& player);
};
