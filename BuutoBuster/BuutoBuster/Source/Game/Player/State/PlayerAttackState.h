#pragma once
#include "ICharacterState.h"
#include "AttackData.h"
#include <vector>

class Player;
class Character;

// 斬り 強斬り 対空斬り ヘビーアタック はすべてこの形で、数値だけが違う
class PlayerAttackState : public ICharacterState<Player> {
private:
    // 攻撃の向きを吸い付ける範囲
    static constexpr float AIM_RADIUS = 420.0f;

    // 対空斬りで跳ぶ強さ
    static constexpr float ANTI_AIR_JUMP_SPEED = 620.0f;

    const AttackData& _data;

    // 通常の斬りの何段目か 強斬りなど段のない技は -1
    int _comboIndex;
    bool _isAntiAir;

    // 溜めてから振ったときの段階 溜めていない技は -1
    // 溜めから来たときは、溜めで止めた振りかぶりの続きから振る
    int _chargeLevel;

    std::vector<Character*> _hitList;
    bool _isFirstFrame = true;
    bool _hasEnteredSecondHit = false;
    bool _hasPlayedSwing = false;

    // 振っている最中に押された次の技 振り終わったら出す
    Technique _queued = Technique::None;

public:
    PlayerAttackState(const AttackData& data, int comboIndex, bool isAntiAir, int chargeLevel = -1);

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override;

private:
    void PlaySwingEffects(Player& player, float time);
    void ApplyHit(Player& player);
    bool TryContinue(Player& player, const InputInfo& input);
};
