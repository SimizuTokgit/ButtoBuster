#pragma once
#include "Component.h"
#include "Collision.h"
#include "Transform.h"
#include "PhysicsManager.h"
#include "GameObject.h"
#include "DxLib.h"
#include <functional>

// 前方宣言
class Collider;

/// <summary>
/// コライダー基底クラス
/// CapsuleCollider, MeshCollider の基底となる
/// </summary>
class Collider : public Component {
public:
    /// <summary>
    /// true: コライダーが有効
    /// false: コライダーが無効（当たり判定を行わない）
    /// </summary>
    bool enabled = true;

    /// <summary>
    /// true: 物理的にすり抜ける（イベントのみ発火）
    /// false: 物理的に衝突する
    /// </summary>
    bool isTrigger = false;

    /// <summary>ローカル座標でのオフセット</summary>
    VECTOR center = VGet(0, 0, 0);

    /// <summary>コリジョンコールバック関数型（物理衝突用）</summary>
    using CollisionCallback = std::function<void(const Collision&)>;

    /// <summary>トリガーコールバック関数型（Collider*を引数）</summary>
    using TriggerCallback = std::function<void(Collider*)>;

protected:
    CollisionCallback _onCollisionEnter;
    CollisionCallback _onCollisionStay;
    CollisionCallback _onCollisionExit;
    TriggerCallback _onTriggerEnter;
    TriggerCallback _onTriggerStay;
    TriggerCallback _onTriggerExit;

    bool _isRegistered = false;

public:
    Collider() = default;

    ~Collider() override {
        Unregister();
    }

    /// <summary>ワールド座標での中心位置を取得</summary>
    virtual VECTOR GetWorldCenter() const {
        if (transform) {
            return VAdd(transform->position, center);
        }
        return center;
    }

    // コールバック登録メソッド（物理衝突用）
    void SetOnCollisionEnter(CollisionCallback callback) { _onCollisionEnter = std::move(callback); }
    void SetOnCollisionStay(CollisionCallback callback) { _onCollisionStay = std::move(callback); }
    void SetOnCollisionExit(CollisionCallback callback) { _onCollisionExit = std::move(callback); }

    // コールバック登録メソッド（トリガー用）
    void SetOnTriggerEnter(TriggerCallback callback) { _onTriggerEnter = std::move(callback); }
    void SetOnTriggerStay(TriggerCallback callback) { _onTriggerStay = std::move(callback); }
    void SetOnTriggerExit(TriggerCallback callback) { _onTriggerExit = std::move(callback); }

    // PhysicsManagerから呼び出されるコールバック実行メソッド
    // コールバック関数とMonoBehaviourの仮想関数の両方を呼び出す

    void InvokeOnCollisionEnter(const Collision& collision) {
        if (_onCollisionEnter) _onCollisionEnter(collision);
        if (gameObject) gameObject->OnCollisionEnter(collision);
    }

    void InvokeOnCollisionStay(const Collision& collision) {
        if (_onCollisionStay) _onCollisionStay(collision);
        if (gameObject) gameObject->OnCollisionStay(collision);
    }

    void InvokeOnCollisionExit(const Collision& collision) {
        if (_onCollisionExit) _onCollisionExit(collision);
        if (gameObject) gameObject->OnCollisionExit(collision);
    }

    void InvokeOnTriggerEnter(Collider* other) {
        if (_onTriggerEnter) _onTriggerEnter(other);
        if (gameObject) gameObject->OnTriggerEnter(other);
    }

    void InvokeOnTriggerStay(Collider* other) {
        if (_onTriggerStay) _onTriggerStay(other);
        if (gameObject) gameObject->OnTriggerStay(other);
    }

    void InvokeOnTriggerExit(Collider* other) {
        if (_onTriggerExit) _onTriggerExit(other);
        if (gameObject) gameObject->OnTriggerExit(other);
    }

    /// <summary>PhysicsManagerへの登録処理</summary>
    void Register() {
        if (!_isRegistered) {
            PhysicsManager::Instance().RegisterCollider(this);
            _isRegistered = true;
        }
    }

    /// <summary>PhysicsManagerからの登録解除処理</summary>
    void Unregister() {
        if (_isRegistered) {
            PhysicsManager::Instance().UnregisterCollider(this);
            _isRegistered = false;
        }
    }
};
