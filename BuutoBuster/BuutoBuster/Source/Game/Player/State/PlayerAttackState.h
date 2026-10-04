#pragma once
#include "ICharacterState.h"
#include "AttackData.h"
#include <memory>
#include <vector>

class Player;
class Character;
class BlowChain;

// 斬り 強斬り 対空斬り 空中の斬り はすべてこの形で、数値だけが違う
class PlayerAttackState : public ICharacterState<Player> {
public:
    // どこで振る技か 次の段へのつなぎ方と、宙での動き方が変わる
    enum class Kind {
        Ground,     // 地上の斬り 強斬り ヘビーアタック
        AntiAir,    // 跳びながらの斬り上げ
        Air,        // 空中の斬り
    };

private:
    // 攻撃の向きを吸い付ける範囲
    static constexpr float AIM_RADIUS = 420.0f;

    // 対空斬りで跳ぶ強さ
    static constexpr float ANTI_AIR_JUMP_SPEED = 620.0f;

    // 空中で振るときに浮き直す速さと、振っている間に落ちる速さの上限
    static constexpr float AIR_HANG_SPEED = 150.0f;
    static constexpr float AIR_FALL_SPEED = 120.0f;

    // ジャスト回避のあとなら反撃に書き換えるので、技の数値は写しを持つ
    AttackData _data;

    // 通常の斬りの何段目か 強斬りなど段のない技は -1
    int _comboIndex;
    Kind _kind;

    // 溜めてから振ったときの段階 溜めていない技は -1
    // 溜めから来たときは、溜めで止めた振りかぶりの続きから振る
    int _chargeLevel;

    std::vector<Character*> _hitList;

    // この振りで吹き飛ばした敵が入る連鎖 振るたびに新しく作る
    std::shared_ptr<BlowChain> _chain;

    bool _isFirstFrame = true;
    bool _hasEnteredSecondHit = false;
    bool _hasPlayedSwing = false;

    // 空中で浮き直したか 浮き直した振りの間だけ、ゆっくり落ちる
    bool _isHanging = false;

    // 振っている最中に押された次の技 振り終わったら出す
    Technique _queued = Technique::None;

public:
    PlayerAttackState(const AttackData& data, int comboIndex, Kind kind, int chargeLevel = -1);

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override;

private:
    void PlaySwingEffects(Player& player, float time);
    void ApplyHit(Player& player);
    bool TryContinue(Player& player, const InputInfo& input);
};
