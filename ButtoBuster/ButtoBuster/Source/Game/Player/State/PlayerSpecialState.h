#pragma once
#include "ICharacterState.h"

class Player;

// 必殺技 (パッド Y / キーボード F) ゲージが満タンのときだけ入る
// 溜めと同じ姿勢で剣を掲げて雷を呼び、場にいる敵全員に雷を落としてから、剣を振り抜く
// 呼んでいる間と、落ちてから動けるまでの間は攻撃を受けない 時間は PlayerData の special で始まる値
//
// 雷の当たり方は PlayerAttacks の GetSpecial 見た目と音は、知らせ (SpecialEvent) を受けた Observer が出す
class PlayerSpecialState : public ICharacterState<Player> {
private:
    // 振り抜き終わるフレーム 刃の軌跡をここまで出す 姿勢と同じ Attack3 のフレーム
    static constexpr float SWING_END_TIME = 13.0f;

    enum class Phase {
        Call,       // 剣を掲げて雷を呼ぶ
        Recover,    // 雷が落ちたあと、振り抜いて動けるようになるまで
    };

    Phase _phase = Phase::Call;
    float _timer = 0.0f;

    // 振り抜いている間に押された次の技 動けるようになったら出す
    Technique _queued = Technique::None;

public:
    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override { return "Special"; }

private:
    // 場にいる敵全員に雷を落とす
    void Strike(Player& player);

    void HoldPose(Player& player) const;
};
