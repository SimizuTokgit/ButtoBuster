#pragma once
#include "Character.h"
#include "StateManager.h"
#include "Observer.h"
#include "PlayerChargeEvent.h"
#include "ChainEvent.h"
#include "JustDodgeEvent.h"
#include "PlayerData.h"

class SlashTrail;

// 操作するキャラ
// 何をするかは状態クラスが決め、ここには状態から使われる窓口をまとめる
class Player : public Character {
public:
    // 動きと手応えの数値 各状態はここから読み、強化はここを書き換える
    PlayerData data;

    // デバッグの無敵
    bool isCheatInvincible = false;

private:
    StateManager<Player> _states;
    SlashTrail* _trail = nullptr;

    Subject<PlayerChargeEvent> _chargeEvents;
    Subject<ChainEvent> _chainEvents;
    Subject<JustDodgeEvent> _justDodgeEvents;

    bool _isGuarding = false;
    bool _isGuardImpact = false;

    int _combo = 0;
    float _comboTimer = 0.0f;
    int _maxCombo = 0;

    // ジャスト回避になる残りの時間と、次の攻撃が反撃になる残りの時間
    float _justDodgeTimer = 0.0f;
    float _counterTimer = 0.0f;

    // 着地までに、あと何回空中で浮き直せるか
    int _airHangLeft = 0;

    VECTOR _spawnPosition = VGet(0.0f, 0.0f, 0.0f);

public:
    void Start() override;
    void Execute(const InputInfo& input, float deltaTime) override;
    HitResult TakeHit(const HitInfo& info) override;

    StateManager<Player>& GetStates() { return _states; }
    const char* GetStateName() const override { return _states.GetCurrentName(); }

    void SetTrail(SlashTrail* trail) { _trail = trail; }
    void SetTrailEmitting(bool isEmitting);

    // 溜めで起きたことを知らせる先 音やエフェクトはここに Observer として登録する
    Subject<PlayerChargeEvent>& GetChargeEvents() { return _chargeEvents; }

    // 自分の振りから始まった連鎖ぶっ飛ばしを知らせる先 画面の表示 音 エフェクトが登録する
    Subject<ChainEvent>& GetChainEvents() { return _chainEvents; }

    // ジャスト回避が決まったことを知らせる先 スロー 音 画面の文字が登録する
    Subject<JustDodgeEvent>& GetJustDodgeEvents() { return _justDodgeEvents; }

    // 回避を始めたときに呼ぶ ここから少しのうちに来た攻撃はジャスト回避になる
    void OpenJustDodgeWindow() { _justDodgeTimer = data.justDodgeWindow; }

    // ジャスト回避のあとの反撃を使う 使えたら true 技を始めるときに呼び、使えたらその技を反撃にする
    bool ConsumeCounter();
    bool HasCounter() const { return _counterTimer > 0.0f; }

    // 空中で浮き直す 着地までの回数が残っていれば 1 回使って true
    bool TryUseAirHang();

    void SetGuarding(bool isGuarding) { _isGuarding = isGuarding; }
    bool IsGuarding() const { return _isGuarding; }

    // ガードで受けたことを1回だけ知らせる ガード状態がのけぞりのアニメに使う
    bool ConsumeGuardImpact();

    void AddCombo(int hits);
    int GetCombo() const { return _combo; }
    float GetComboTimer() const { return _comboTimer; }
    int GetMaxCombo() const { return _maxCombo; }

    void HealFull();

    // 攻撃の向きを決める
    // 入力の向きから大きく外れない範囲で、近くの敵に少しだけ吸い付ける
    VECTOR FindAimDirection(VECTOR inputDirection, float searchRadius) const;

    // 見た目を半透明にする 回避の無敵中に使う
    void SetOpacity(float rate);

private:
    void UpdateCombo(float deltaTime);
    void UpdateJustDodge(float deltaTime);
    void SucceedJustDodge();

    // 地形の穴に落ちたら、最初の場所へ戻す
    void ReturnIfFallen();
};
