#pragma once
#include "Character.h"
#include "StateManager.h"
#include "EnemyData.h"

// 敵
// どの種類も同じクラスで、違いは EnemyData の数値と使うアニメだけ
// 何をするかは EnemyAI が InputInfo で伝える プレイヤーと同じ入口を通る
class Enemy : public Character {
public:
    // 倒れてから沈み始めるまで
    static constexpr float CORPSE_TIME = 1.2f;

    // 地面に沈みきるまで
    static constexpr float SINK_TIME = 0.8f;

    // 斬られてから頭上の体力を出しておく時間
    static constexpr float HP_BAR_TIME = 3.0f;

private:
    const EnemyData* _data = nullptr;
    int _id = 0;
    StateManager<Enemy> _states;
    Character* _target = nullptr;

    bool _isHovering = false;
    bool _hasFinishedAttack = false;
    bool _isReadyToRemove = false;
    bool _isCounted = false;
    float _hpBarTimer = 0.0f;

public:
    void Initialize(const EnemyData& data, int id, Character* target);

    void Start() override;
    void Execute(const InputInfo& input, float deltaTime) override;
    HitResult TakeHit(const HitInfo& info) override;

    const EnemyData& GetData() const { return *_data; }
    int GetId() const { return _id; }
    Character* GetTarget() const { return _target; }

    StateManager<Enemy>& GetStates() { return _states; }
    const char* GetStateName() const override { return _states.GetCurrentName(); }

    // 攻撃が終わったことを AI に1回だけ知らせる 攻撃の番を返すきっかけ
    void NotifyAttackFinished() { _hasFinishedAttack = true; }
    bool ConsumeAttackFinished();

    // 空を飛ぶ敵が高さを保つかどうか 落とされている間は止める
    void SetHovering(bool isHovering);
    bool IsHovering() const { return _isHovering; }

    // 倒れて沈み終わった もう片付けてよい
    bool IsReadyToRemove() const { return _isReadyToRemove; }
    void MarkReadyToRemove() { _isReadyToRemove = true; }

    // 撃破数を1回だけ数えるため
    bool TryCountDefeat();

    float GetHpBarTimer() const { return _hpBarTimer; }

    // 見た目ごと透けさせる 沈むときに使う
    void SetOpacity(float rate);

private:
    void KeepHovering(float deltaTime);
};
