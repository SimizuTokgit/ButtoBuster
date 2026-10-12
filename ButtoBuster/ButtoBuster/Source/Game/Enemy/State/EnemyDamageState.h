#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Enemy;

class EnemyDamageState : public ICharacterState<Enemy> {
private:
    // 叩きつけで落とされたときに、地面で跳ね上がる速さ
    static constexpr float SLAM_BOUNCE_SPEED = 800.0f;

    VECTOR _knockback;

    // 叩きつけで真下へ落とされたか 地面に着いたら 1 回跳ね上がる
    bool _isSlammed;

public:
    explicit EnemyDamageState(VECTOR knockback, bool isSlammed = false)
        : _knockback(knockback)
        , _isSlammed(isSlammed) {
    }

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Damage"; }
};
