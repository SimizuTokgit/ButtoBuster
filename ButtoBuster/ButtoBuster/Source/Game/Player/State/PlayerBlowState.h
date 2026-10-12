#pragma once
#include "ICharacterState.h"
#include "DxLib.h"

class Player;

// 吹き飛んで倒れ、起き上がるまで 壁にぶつかったら一瞬張り付いてから跳ね返る
// 跳ねる速さ 倒れている時間 起き上がったあとの無敵は PlayerData 壁で跳ね返る強さと張り付く時間は ArenaWall
class PlayerBlowState : public ICharacterState<Player> {
private:
    // 起き上がるまでの無敵の秒数 起き上がったら getUpInvincibleTime に戻す
    static constexpr float DOWN_INVINCIBLE_TIME = 3.0f;

    // 飛んでいる間、水平の速さを毎秒この割合で減らす
    static constexpr float DAMP_RATE = 2.0f;

    // 着地したときに立てる砂ぼこりの数
    static constexpr int DUST_COUNT = 10;

    // 起き上がるアニメ (BlowOut) の速さ
    static constexpr float GET_UP_ANIMATION_SPEED = 1.3f;

    enum class Phase {
        Fly,
        Stick,      // 壁に張り付いている 時間が来たら跳ね返って Fly に戻る (Character::HoldOnWall)
        Down,
        GetUp,
    };

    VECTOR _knockback;
    Phase _phase = Phase::Fly;
    float _timer = 0.0f;

public:
    explicit PlayerBlowState(VECTOR knockback) : _knockback(knockback) {}

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    const char* GetName() const override { return "Blow"; }

    // 吹き飛んで落ちていく間に毎フレーム呼ぶ 場外へ飛ばされたとき (PlayerDeadState) も同じ落ち方をする
    // 滑りを弱めていき、吹き飛ぶアニメが終わって着地したら、倒れた姿にして砂ぼこりを dustCount 個立てる 倒れたら true
    static bool FallDown(Player& player, int dustCount, float deltaTime);
};
