#pragma once
#include "ICharacterState.h"
#include "DxLib.h"
#include <memory>

class Enemy;
class BlowChain;

// 吹き飛んで倒れ、起き上がるまで
// 空を飛ぶ敵は地面まで落ちて、しばらくしてから飛び直す
// 吹き飛ぶアニメを持たない重い敵は、構えたまま押し飛ばされ、着地したら少しして動き出す
//
// 連鎖を持って飛んでいる間は砲弾になり、触れた敵を巻き込む
class EnemyBlowState : public ICharacterState<Enemy> {
private:
    static constexpr float DOWN_TIME = 0.6f;

    // 落とされた Bee が地面にいる時間 ここで地上の斬りが届く
    static constexpr float GROUNDED_TIME = 1.4f;

    // 重い敵が押し飛ばされるときの上向きの速さと、着地してから動き出すまでの時間
    static constexpr float HEAVY_JUMP_SPEED = 250.0f;
    static constexpr float HEAVY_RECOVER_TIME = 0.5f;

    enum class Phase {
        Fly,
        Down,
        GetUp,
    };

    VECTOR _knockback;
    std::shared_ptr<BlowChain> _chain;
    Phase _phase = Phase::Fly;
    float _timer = 0.0f;

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
