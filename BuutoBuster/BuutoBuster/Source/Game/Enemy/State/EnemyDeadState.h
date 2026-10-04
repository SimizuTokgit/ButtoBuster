#pragma once
#include "ICharacterState.h"
#include "DxLib.h"
#include <memory>

class Enemy;
class BlowChain;

// 倒れて、しばらく横たわってから地面に沈む
// 倒れながら飛ばされている間も砲弾になり、触れた敵を巻き込む 壁にぶつかったら跳ね返る
class EnemyDeadState : public ICharacterState<Enemy> {
private:
    // 重い敵が飛ばされながら倒れるときの上向きの速さ
    static constexpr float HEAVY_JUMP_SPEED = 250.0f;

    enum class Phase {
        Fall,
        Lie,
        Sink,
    };

    VECTOR _knockback;
    std::shared_ptr<BlowChain> _chain;

    // 飛ばされながら倒れるか 剣では止まらない重い敵は、飛んできた敵に倒されたときだけ飛ぶ
    bool _isBlown;

    Phase _phase = Phase::Fall;
    float _timer = 0.0f;

public:
    EnemyDeadState(VECTOR knockback, std::shared_ptr<BlowChain> chain, bool isBlown)
        : _knockback(knockback)
        , _chain(std::move(chain))
        , _isBlown(isBlown) {
    }

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Dead"; }

private:
    void LeaveChain(Enemy& enemy);
};
