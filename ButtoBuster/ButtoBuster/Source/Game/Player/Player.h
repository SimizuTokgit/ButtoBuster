#pragma once
#include "Character.h"
#include "StateManager.h"
#include "Observer.h"
#include "PlayerChargeEvent.h"
#include "ChainEvent.h"
#include "JustDodgeEvent.h"
#include "SpecialEvent.h"
#include "PlayerData.h"

class SlashTrail;

// 操作するキャラ
// 何をするかは状態クラスが決め、ここには状態から使われる窓口をまとめる
class Player : public Character {
public:
    // 動きと手応えの数値 各状態はここから読み、強化はここを書き換える
    PlayerData data;

    // デバッグの無敵 攻撃を受けず、壁でも割られない
    bool isCheatInvincible = false;

    // 負けるかどうか チュートリアルの練習中は false 攻撃は受けるが、許容値に届いて壁へ飛ばされても割れずに跳ね返る
    bool canLose = true;

private:
    StateManager<Player> _states;
    SlashTrail* _trail = nullptr;

    Subject<PlayerChargeEvent> _chargeEvents;
    Subject<ChainEvent> _chainEvents;
    Subject<JustDodgeEvent> _justDodgeEvents;
    Subject<SpecialEvent> _specialEvents;

    bool _isGuarding = false;
    bool _isGuardImpact = false;

    int _combo = 0;
    float _comboTimer = 0.0f;
    int _maxCombo = 0;

    // ジャスト回避になる残りの時間と、攻撃が反撃になる残りの時間
    float _justDodgeTimer = 0.0f;
    float _counterTimer = 0.0f;

    // 反撃の間、敵全員をゆっくりにしている残りの時間 溜めている間も減る
    float _slowTimer = 0.0f;

    // 着地までに、あと何回空中で浮き直せるか
    int _airHangLeft = 0;

    // 回避の残り 1 で 1 回分 端数は戻っている途中の分
    float _dodgeStock = 0.0f;

    // 必殺技のゲージ 0〜data.specialGaugeMax と、撃ってからまた溜まり始めるまでの残り 秒
    float _specialGauge = 0.0f;
    float _specialLockTimer = 0.0f;

    // ガードで受けた回数と、攻撃を食らった回数 チュートリアルが数える
    int _guardCount = 0;
    int _hitCount = 0;

    VECTOR _spawnPosition = VGet(0.0f, 0.0f, 0.0f);

public:
    void Start() override;
    void Execute(const InputInfo& input, float deltaTime) override;
    HitResult TakeHit(const HitInfo& info) override;

    StateManager<Player>& GetStates() { return _states; }
    const char* GetStateName() const override { return _states.GetCurrentName(); }

    // 壁を割られたときに呼ばれる 場外へ飛ばされて負け
    void Defeat(VECTOR knockback) override;

    // デバッグの無敵中と、負けない練習中 (canLose が false) は、壁を割られずに跳ね返る
    bool CanBeDefeated() const override { return !isCheatInvincible && canLose; }

    // ジャスト回避のあとの反撃の間は、敵全員をゆっくりにする
    float GetOpponentTimeScale() const override;

    // あと何秒、回避もできないか 今の状態に聞く
    float GetOpeningTime() const override;

    void SetTrail(SlashTrail* trail) { _trail = trail; }
    void SetTrailEmitting(bool isEmitting);

    // 溜めで起きたことを知らせる先 音やエフェクトはここに Observer として登録する
    Subject<PlayerChargeEvent>& GetChargeEvents() { return _chargeEvents; }

    // 自分の振りから始まった連鎖ぶっ飛ばしを知らせる先 画面の表示 音 エフェクトが登録する
    Subject<ChainEvent>& GetChainEvents() { return _chainEvents; }

    // ジャスト回避が決まったことを知らせる先 画面の演出 音 画面の文字が登録する
    Subject<JustDodgeEvent>& GetJustDodgeEvents() { return _justDodgeEvents; }

    // 必殺技で起きたことを知らせる先 ゲージが満タンになった 雷を呼んだ 雷が落ちた 画面の演出と音が登録する
    Subject<SpecialEvent>& GetSpecialEvents() { return _specialEvents; }

    // 回避を始めたときに呼ぶ ここから少しのうちに来た攻撃はジャスト回避になる
    void OpenJustDodgeWindow() { _justDodgeTimer = data.justDodgeWindow; }

    // 回避の残りが 1 回分以上あるか 無ければ回避は出ない
    bool CanDodge() const { return _dodgeStock >= 1.0f; }

    // 回避を始めたときに呼ぶ 残りを 1 回分減らす
    void UseDodge();

    // 回避の残り 1 で 1 回分 端数は戻っている途中の分 画面のバーが読む
    float GetDodgeStock() const { return _dodgeStock; }

    // 必殺技のゲージを足す 撃ってすぐの間 (data.specialRefillDelay) は溜まらない
    // 当てた数は AddCombo から、連鎖と壁割りは SpecialGaugeObserver から足す
    void AddSpecialGauge(float amount);

    // デバッグ用 撃ってすぐでも、ゲージを満タンにする
    void FillSpecialGauge();

    // 必殺技のゲージが満タンか 満タンでなければ必殺技は出ない
    bool IsSpecialReady() const { return _specialGauge >= data.specialGaugeMax; }

    // 必殺技を撃つときに呼ぶ ゲージを空にし、しばらく溜まらないようにする
    void UseSpecial();

    // 必殺技のゲージの溜まり具合 0〜1 画面のゲージが読む
    float GetSpecialRatio() const;

    // ジャスト回避のあとの反撃を使い切る 使えたら true
    // 締めの一振りを始めるときに呼び、使えたらその技を反撃の吹き飛ばしにする 敵の時間も元に戻る
    bool ConsumeCounter();

    // 反撃できる間か 締めの前の斬りは、これを見て反撃の重さにする
    bool HasCounter() const { return _counterTimer > 0.0f; }

    // 空中で浮き直す 着地までの回数が残っていれば 1 回使って true
    bool TryUseAirHang();

    void SetGuarding(bool isGuarding) { _isGuarding = isGuarding; }
    bool IsGuarding() const override { return _isGuarding; }

    // 溜め (PlayerChargeState) の最中か 中身は Player.cpp の最後
    bool IsCharging() const override;

    // ガードで受けたことを1回だけ知らせる ガード状態がのけぞりのアニメに使う
    bool ConsumeGuardImpact();

    // ガードで受けた回数と、攻撃を食らった回数 チュートリアルが、できたかどうかを数えるのに使う
    int GetGuardCount() const { return _guardCount; }
    int GetHitCount() const { return _hitCount; }

    void AddCombo(int hits);
    int GetCombo() const { return _combo; }
    float GetComboTimer() const { return _comboTimer; }
    int GetMaxCombo() const { return _maxCombo; }

    // 決まったフェーズごとの区切りで呼ぶ 吹っ飛ばされ値を 0 に戻す
    void HealFull();

    // 攻撃の向きを決める
    // 入力の向きから大きく外れない範囲で、近くの敵に少しだけ吸い付ける
    VECTOR FindAimDirection(VECTOR inputDirection, float searchRadius) const;

    // 見た目を半透明にする 回避の無敵中に使う
    void SetOpacity(float rate);

protected:
    const BlowSettings& GetBlowSettings() const override { return data.blow; }

private:
    void UpdateCombo(float deltaTime);
    void UpdateJustDodge(float deltaTime);
    void UpdateDodgeStock(float deltaTime);

    // attacker はかわした攻撃を振った相手 その目の前まで寄る 分からなければ寄らない
    void SucceedJustDodge(const Character* attacker);

    // ガードで止めた攻撃の持ち主をのけぞらせる 反撃の隙を作る
    void StaggerGuardedAttacker(const HitInfo& info);

    // 地形の穴に落ちたら、最初の場所へ戻す
    void ReturnIfFallen();
};
