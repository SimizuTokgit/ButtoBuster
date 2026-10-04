#pragma once
#include "MonoBehaviour.h"
#include "InputInfo.h"
#include "HitInfo.h"
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
// 体力 移動 向き アニメの流し方をまとめておく
// 何をするかは派生クラスの状態が決める
class Character : public MonoBehaviour {
public:
    Team team = Team::Enemy;
    int maxHp = 100;
    int hp = 100;

    // 攻撃の届く距離の計算に使う 当たり判定のカプセルから取る
    float bodyRadius = 30.0f;
    float bodyHeight = 160.0f;

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

private:
    static constexpr float FLASH_TIME = 0.12f;

    void UpdateFlash();
};
