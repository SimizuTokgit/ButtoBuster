#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Enemy;

class EnemyDamageState : public ICharacterState<Enemy> {
private:
    // のけぞるアニメの速さ
    static constexpr float ANIMATION_SPEED = 1.2f;

    // 押される速さ 当たった技の吹っ飛ばしにこの割合を掛ける
    static constexpr float KNOCKBACK_RATE = 0.6f;

    // 押されたあと滑って止まる 水平の速さを毎秒この割合で減らす
    static constexpr float DAMP_RATE = 8.0f;

    // のけぞるアニメのこのフレームを過ぎたら、終わりまで待たずに動き出す
    static constexpr float RECOVER_FRAME = 12.0f;

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
