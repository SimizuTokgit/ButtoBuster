#pragma once
#include "Character.h"
#include "StateManager.h"
#include "Observer.h"
#include "PlayerChargeEvent.h"
#include "ChainEvent.h"
#include "JustDodgeEvent.h"

class SlashTrail;

// 操作するキャラ
// 何をするかは状態クラスが決め、ここには状態から使われる窓口をまとめる
class Player : public Character {
public:
    static constexpr int MAX_HP = 100;
    static constexpr float MOVE_SPEED = 600.0f;
    static constexpr float TURN_SPEED = 900.0f;

    // 被弾してから次に食らうまでの猶予
    // 囲まれて起き上がれないまま削り切られるのを防ぐ
    static constexpr float HURT_INVINCIBLE_TIME = 0.6f;

    // 正面から左右にどこまでの攻撃を防げるか 1 で真正面だけ
    static constexpr float GUARD_DOT = 0.2f;

    // コンボが途切れるまでの時間
    static constexpr float COMBO_KEEP_TIME = 2.5f;

    // 回避を始めてからこの秒数のうちに攻撃が来たら、ジャスト回避になる
    static constexpr float JUST_DODGE_WINDOW = 0.15f;

    // ジャスト回避が決まったあとの無敵 スローの間に続けて来た攻撃も受けない
    static constexpr float JUST_DODGE_INVINCIBLE_TIME = 0.6f;

    // ジャスト回避のあと、次の攻撃が反撃になる時間
    static constexpr float COUNTER_TIME = 1.5f;

    // 着地までに空中で浮き直せる回数 空中の斬り 1 回分
    static constexpr int AIR_HANG_COUNT = 3;

    // デバッグの無敵
    bool isCheatInvincible = false;

    // 戦える範囲 外に出ようとしたら押し戻す
    VECTOR arenaCenter = VGet(0.0f, 0.0f, 0.0f);
    float arenaRadius = 2200.0f;

    // 溜めで止める姿勢 アニメの名前と止める時間
    // 始めは PlayerChargeState に書いた値 デバッグの姿勢探しで、遊びながら書き換えられる
    std::string chargePoseAnimation;
    float chargePoseTime = 0.0f;

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
    int _airHangLeft = AIR_HANG_COUNT;

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
    void OpenJustDodgeWindow() { _justDodgeTimer = JUST_DODGE_WINDOW; }

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
    void KeepInsideArena();
};
