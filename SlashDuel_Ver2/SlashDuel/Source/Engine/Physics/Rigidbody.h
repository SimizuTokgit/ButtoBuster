#pragma once
#include "Component.h"
#include "Transform.h"
#include "PhysicsManager.h"
#include "DxLib.h"

/// <summary>
/// 物理演算を行うコンポーネント
/// 重力、速度、衝突応答などを管理
/// </summary>
class Rigidbody : public Component {
private:
    /// <summary>重力加速度（cm/s^2）DxLibは通常cmスケール</summary>
    static constexpr float GRAVITY = 980.0f;

public:
    /// <summary>質量（kg）</summary>
    float mass = 1.0f;

    /// <summary>空気抵抗（線形減衰係数）</summary>
    float drag = 0.0f;

    /// <summary>重力を使用するかどうか</summary>
    bool useGravity = true;

    /// <summary>
    /// true: 物理演算を無視（スクリプトで位置を制御する場合）
    /// false: 物理演算で動く
    /// </summary>
    bool isKinematic = false;

    /// <summary>X軸方向の移動を固定</summary>
    bool freezePositionX = false;

    /// <summary>Y軸方向の移動を固定</summary>
    bool freezePositionY = false;

    /// <summary>Z軸方向の移動を固定</summary>
    bool freezePositionZ = false;

    /// <summary>現在の速度（cm/s）</summary>
    VECTOR linearVelocity = VGet(0, 0, 0);

    bool isGrounded = false; // 地面に接地しているか

private:
    bool _isRegistered = false;

public:
    Rigidbody() = default;

    ~Rigidbody() override {
        Unregister();
    }

    /// <summary>
    /// 力を加える（F = ma に基づく）
    /// </summary>
    /// <param name="force">加える力（ニュートン相当）</param>
    void AddForce(VECTOR force) {
        if (isKinematic) return;
        
        // F = ma より a = F/m
        if (mass > 0.0f) {
            linearVelocity = VAdd(linearVelocity, VScale(force, 1.0f / mass));
        }
    }

    /// <summary>
    /// 衝撃を加える（瞬間的な速度変化）
    /// </summary>
    /// <param name="impulse">加える衝撃</param>
    void AddImpulse(VECTOR impulse) {
        if (isKinematic) return;
        
        // 衝撃は質量に関係なく即座に速度に加算
        linearVelocity = VAdd(linearVelocity, impulse);
    }

    /// <summary>
    /// 物理更新（PhysicsManagerから呼ばれる）
    /// </summary>
    /// <param name="deltaTime">経過時間（秒）</param>
    void PhysicsUpdate(float deltaTime) {
        if (isKinematic) return;

        // 重力を適用
        ApplyGravity(deltaTime);

        // 空気抵抗を適用
        ApplyDrag(deltaTime);

        // 速度制約を適用
        ApplyConstraints();

        // 位置を更新
        transform->localPosition = VAdd(transform->localPosition, VScale(linearVelocity, deltaTime));
    }

    /// <summary>PhysicsManagerへの登録処理</summary>
    void Register() {
        if (!_isRegistered) {
            PhysicsManager::Instance().RegisterRigidbody(this);
            _isRegistered = true;
        }
    }

    /// <summary>PhysicsManagerからの登録解除処理</summary>
    void Unregister() {
        if (_isRegistered) {
            PhysicsManager::Instance().UnregisterRigidbody(this);
            _isRegistered = false;
        }
    }

private:
    /// <summary>重力を適用</summary>
    void ApplyGravity(float deltaTime) {
        if (!useGravity) return;

        // 重力加速度を速度に加算
        linearVelocity.y -= GRAVITY * deltaTime;
    }

    /// <summary>空気抵抗を適用</summary>
    void ApplyDrag(float deltaTime) {
        if (drag <= 0.0f) return;

        // 線形減衰: v = v * (1 - drag * dt)
        float damping = 1.0f - drag * deltaTime;
        if (damping < 0.0f) damping = 0.0f;

        linearVelocity = VScale(linearVelocity, damping);
    }

    /// <summary>速度制約を適用</summary>
    void ApplyConstraints() {
        if (freezePositionX) linearVelocity.x = 0.0f;
        if (freezePositionY) linearVelocity.y = 0.0f;
        if (freezePositionZ) linearVelocity.z = 0.0f;
    }
};
