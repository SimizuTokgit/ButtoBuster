#pragma once
#include "ICharacterState.h"
#include "ArenaWall.h"
#include "DxLib.h"
#include <memory>

class Enemy;
class BlowChain;

// 吹き飛んで倒れ、起き上がるまで
// 空を飛ぶ敵は地面まで落ちて、しばらくしてから飛び直す
// 吹き飛ぶアニメを持たない重い敵は、構えたまま押し飛ばされ、着地したら少しして動き出す
//
// 連鎖を持って飛んでいる間は砲弾になり、触れた敵を巻き込む
// 戦える範囲の壁にぶつかったら、一瞬張り付いてから跳ね返る 速くぶつかれば、戻ってくる間も砲弾のまま
// 吹っ飛ばされ値が許容値に届いていれば、跳ね返らずに壁を割って場外へ飛ぶ (撃破)
class EnemyBlowState : public ICharacterState<Enemy> {
private:
    static constexpr float DOWN_TIME = 0.6f;

    // 落とされた Bee が地面にいる時間 ここで地上の斬りが届く
    static constexpr float GROUNDED_TIME = 1.4f;

    // 重い敵が押し飛ばされるときの上向きの最低の速さと、着地してから動き出すまでの時間
    static constexpr float HEAVY_JUMP_SPEED = 250.0f;
    static constexpr float HEAVY_RECOVER_TIME = 0.5f;

    enum class Phase {
        Fly,
        Stick,      // 壁に張り付いている 時間が来たら跳ね返って Fly に戻る
        Down,
        GetUp,
    };

    VECTOR _knockback;
    std::shared_ptr<BlowChain> _chain;
    Phase _phase = Phase::Fly;
    float _timer = 0.0f;

    // 張り付いた壁の様子と、張り付いている秒数 跳ね返すときに使う
    ArenaWall::Hit _wallHit;
    float _stickTime = 0.0f;

public:
    EnemyBlowState(VECTOR knockback, std::shared_ptr<BlowChain> chain)
        : _knockback(knockback)
        , _chain(std::move(chain)) {
    }

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    void Exit(Enemy& enemy) override;
    const char* GetName() const override { return "Blow"; }

private:
    // 砲弾をやめる 着地したときと、飛んでいる途中でほかの状態に移るとき
    void LeaveChain(Enemy& enemy);
};
