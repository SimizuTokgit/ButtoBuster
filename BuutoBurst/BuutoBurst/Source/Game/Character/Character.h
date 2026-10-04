#pragma once
#include "MonoBehaviour.h"
#include "InputInfo.h"
#include "HitInfo.h"
#include "ArenaWall.h"
#include "BlowSettings.h"
#include "DxLib.h"
#include <string>

class Animator;
class SkinnedMeshRenderer;
class Rigidbody;
class CapsuleCollider;

enum class Team {
    Player,
    Enemy,
};

// プレイヤーと敵に共通する体
// 体力 吹っ飛ばされ値 移動 向き アニメの流し方と、戦える範囲の壁の内側に収めるところをまとめておく
// 何をするかは派生クラスの状態が決める
class Character : public MonoBehaviour {
public:
    Team team = Team::Enemy;
    int maxHp = 100;
    int hp = 100;

    // 攻撃の届く距離の計算に使う 当たり判定のカプセルから取る
    float bodyRadius = 30.0f;
    float bodyHeight = 160.0f;

    // 重さ 飛んできた敵とぶつかったとき、どれだけ飛ぶか、どれだけ飛ばすかに使う
    float weight = 1.0f;

protected:
    Animator* _animator = nullptr;
    SkinnedMeshRenderer* _renderer = nullptr;
    Rigidbody* _rigidbody = nullptr;
    CapsuleCollider* _body = nullptr;

    std::string _animationName;
    float _animationSpeed = 1.0f;

    float _invincibleTimer = 0.0f;

    // 攻撃を受けた直後に体を白く光らせる時間
    float _flashTimer = 0.0f;

    // このフレームに壁へぶつかった様子 吹き飛んでいる状態が受け取って跳ね返す
    ArenaWall::Hit _wallHit;
    bool _hasWallHit = false;

    // 吹っ飛ばされ値と、最後に当たってからの時間
    float _blowValue = 0.0f;
    float _blowIdleTime = 0.0f;

    // 次の湯気を出すまでの貯め 1 を超えた分だけ粒を出す
    float _steamTimer = 0.0f;

public:
    ~Character() override;

    // 組み立て役が部品をつないだあとに1回呼ぶ
    void Setup(Animator* animator, SkinnedMeshRenderer* renderer, Rigidbody* rigidbody, CapsuleCollider* body);

    // 操作役から毎フレーム呼ばれる
    // 人でもAIでも同じ入口を通る
    virtual void Execute(const InputInfo& input, float deltaTime) = 0;

    virtual HitResult TakeHit(const HitInfo& info) = 0;

    // デバッグ表示に出す今の状態
    virtual const char* GetStateName() const { return "-"; }

    bool IsDead() const { return hp <= 0; }
    bool IsInvincible() const { return _invincibleTimer > 0.0f; }
    // 無敵の残りを延ばす 短くはしない 別々の理由の無敵が重なったとき長いほうを残す
    void SetInvincible(float seconds);

    // 無敵の残りをこの長さにそろえる 起き上がったあとなど、延ばしすぎた分を戻すときに使う
    void ResetInvincible(float seconds) { _invincibleTimer = seconds; }
    float GetHpRatio() const { return maxHp > 0 ? static_cast<float>(hp) / maxHp : 0.0f; }

    // ----- 吹っ飛ばされ値 -----

    float GetBlowValue() const { return _blowValue; }

    // 許容値に対する割合 1 で許容値に届いた 赤みと湯気の強さもこれで決まる
    float GetBlowRatio() const;

    // 吹き飛んで起き上がったときに呼ぶ 値を少し戻す
    void RecoverBlowOnGetUp();

    void ResetBlow();

    // ----- アニメ -----

    void PlayAnimation(const std::string& name, float speed = 1.0f, bool restart = false);
    float GetAnimationTime() const;
    bool IsAnimationFinished() const;
    void SetAnimationSpeed(float speed) { _animationSpeed = speed; }

    // 今のアニメの再生位置を直接動かす 途中で鳴らす音は鳴らない 姿勢を止めて見せるときに使う
    void SetAnimationTime(float time);

    // ----- 移動 -----

    // 水平の速さだけ決める 上下は重力に任せる
    void SetHorizontalVelocity(VECTOR direction, float speed);
    void StopHorizontal();

    // 吹き飛ばされたときのように、向きと速さをそのまま入れる
    void SetKnockback(VECTOR velocity);

    // 水平の速さを毎秒 rate の割合で減らす 押された後に滑って止まる
    void DampHorizontal(float rate, float deltaTime);

    void SetVerticalVelocity(float speed);
    void SetGravityEnabled(bool isEnabled);

    // 物理を止めて自分で位置を動かす 倒れた敵を地面に沈めるときに使う
    void SetKinematic(bool isKinematic);

    // 体で押し合うかどうか 倒れた敵が生きている敵やプレイヤーを押さないように切る
    // 判定そのものを切ると地形とも当たらなくなって落ちていくので、すり抜けにするだけ
    void SetBodySolid(bool isSolid);
    VECTOR GetVelocity() const;
    bool IsGrounded() const;

    // このフレームに壁へぶつかっていたら、その様子を 1 回だけ渡す 吹き飛んでいる状態が跳ね返すのに使う
    bool ConsumeWallHit(ArenaWall::Hit& outHit);

    // 前の状態のときにぶつかった分を捨てる 吹き飛び始めたときに呼ぶ
    // 外から来た切り替えはフレームの頭で通るので、歩いて壁を押していた分で跳ね返ってしまわないように
    void ForgetWallHit() { _hasWallHit = false; }

    // ----- 向き -----

    void FaceTowards(VECTOR direction, float degreesPerSecond, float deltaTime);
    void FaceImmediately(VECTOR direction);
    VECTOR GetForward() const;

    // 相手の位置が正面からどれだけずれているか 1 で真正面 -1 で真後ろ
    float GetFacingDot(VECTOR worldPosition) const;

    // ----- 位置 -----

    VECTOR GetPosition() const;

    // 胴体の真ん中 エフェクトや飛び道具の狙いに使う
    VECTOR GetCenter() const;

    // 球が体のカプセルに触れているか 飛び道具の当たり判定に使う
    bool IsTouchingSphere(VECTOR center, float radius) const;

    SkinnedMeshRenderer* GetRenderer() const { return _renderer; }
    Animator* GetAnimator() const { return _animator; }

protected:
    void UpdateTimers(float deltaTime);
    void UpdateAnimation(float deltaTime);
    void StartFlash() { _flashTimer = FLASH_TIME; }

    // 壁の内側に収める 状態がこのフレームのうちに跳ね返せるよう、状態の処理より先に呼ぶ
    void UpdateWall();

    // 吹っ飛ばされ値の決まり プレイヤーと敵で持っている場所が違うので、派生クラスが返す
    virtual const BlowSettings& GetBlowSettings() const = 0;

    // 当たった技のダメージの分だけ溜める 減り始めるまでの時間も最初から数え直す
    void AddBlow(float amount);

    // 吹っ飛ばしの向きと速さに、溜まり具合に応じた倍率を掛ける 溜めてから呼ぶ
    VECTOR ScaleKnockback(VECTOR knockback) const;

    // 時間で減らし、溜まり具合に応じて湯気を出す 毎フレーム呼ぶ
    void UpdateBlow(float deltaTime);

private:
    static constexpr float FLASH_TIME = 0.12f;

    // 吹っ飛ばしを伸ばすのは、値が許容値のこの倍まで 伸びすぎて一瞬で場外まで飛ばないように
    // 値もここで頭打ちにする
    static constexpr float KNOCKBACK_RATIO_MAX = 2.0f;

    // 伸ばしたときの速さの上限 速すぎると地形をすり抜ける 技そのものがこれより速ければ、技の速さのまま
    static constexpr float KNOCKBACK_SPEED_MAX = 3600.0f;

    // 赤み 許容値に届いたときに、赤をどれだけ強め、緑と青をどれだけ落とすか
    static constexpr float HEAT_RED = 0.5f;
    static constexpr float HEAT_FADE = 0.55f;

    // 湯気 許容値のこの割合から出始める 出る数は 1 秒に STEAM_PER_SECOND × 割合 (許容値で頭打ち)
    static constexpr float STEAM_START_RATIO = 0.3f;
    static constexpr float STEAM_PER_SECOND = 14.0f;

    // 当たった瞬間の白い光と、溜まり具合の赤みを体の色に出す
    void UpdateBodyColor();
};
