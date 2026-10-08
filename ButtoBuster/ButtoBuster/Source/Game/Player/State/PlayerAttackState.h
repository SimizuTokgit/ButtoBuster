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
    // 強化の倍率を掛けたり、ジャスト回避のあとなら反撃に書き換えたりするので、技の数値は写しを持つ
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

    // 最後の判定が消えてから経った時間 アニメのフレームで数える あと隙が明けたかを見る
    float _recoveryCount = 0.0f;

public:
    PlayerAttackState(const AttackData& data, int comboIndex, Kind kind, int chargeLevel = -1);

    void Enter(Player& player) override;
    void Execute(Player& player, const InputInfo& input, float deltaTime) override;
    void Exit(Player& player) override;
    const char* GetName() const override;
    float GetOpeningTime(const Player& player) const override;

private:
    void PlaySwingEffects(Player& player, float time);
    void ApplyHit(Player& player);
    bool TryContinue(Player& player, const InputInfo& input);

    // あと隙が明けたあと、ガードを押していれば、アニメの終わりを待たずに構える
    bool TryGuard(Player& player, const InputInfo& input);

    // あと隙が明けたら true 明けるまでは回避も次の技も受け付けない
    bool UpdateRecovery(Player& player, float deltaTime);

    // 最後の判定が消えるフレーム 二回斬りなら二回目の終わり あと隙はここから数える
    float GetLastHitEnd() const;
};
