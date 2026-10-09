#pragma once
#include "ICharacterState.h"
#include "AttackData.h"
#include <vector>

class Enemy;
class Character;

// 近接攻撃 どの敵もこの形で、数値だけが違う
class EnemyAttackState : public ICharacterState<Enemy> {
private:
    const AttackData& _data;

    // 0 より大きければ自分の周り全部に当てる
    float _areaRadius;

    std::vector<Character*> _hitList;
    bool _hasEnteredFirstHit = false;
    bool _hasEnteredSecondHit = false;
    bool _hasPlayedArc = false;
    bool _hasPlayedSecondArc = false;
    bool _hasPlayedWarning = false;
    bool _hasPlayedSecondWarning = false;

public:
    EnemyAttackState(const AttackData& data, float areaRadius);

    void Enter(Enemy& enemy) override;
    void Execute(Enemy& enemy, const InputInfo& input, float deltaTime) override;
    void Exit(Enemy& enemy) override;
    const char* GetName() const override { return _areaRadius > 0.0f ? "Stomp" : "Attack"; }

private:
    // 踏みつけの当たる範囲を、振りかぶった瞬間に地面へ出す
    void PlayAreaWarning(Enemy& enemy);

    // 当たる少し前に頭の上を光らせ、音を鳴らす 光った瞬間に回避すればジャスト回避になる
    void PlayWarning(Enemy& enemy, float time);

    void PlaySwing(Enemy& enemy, float time);
    void PlayArc(Enemy& enemy, float swing);
    void ApplyHit(Enemy& enemy, float time);
};
