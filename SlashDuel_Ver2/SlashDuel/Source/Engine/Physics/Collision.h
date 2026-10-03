#pragma once
#include "DxLib.h"

// 前方宣言
class Collider;
class Rigidbody;
class GameObject;
class Transform;

/// <summary>
/// 衝突情報を格納する構造体
/// OnCollisionEnter等のコールバックで使用される
/// </summary>
struct Collision {
    /// <summary>衝突した相手のCollider</summary>
    Collider* collider = nullptr;

    /// <summary>衝突した相手のRigidbody（存在しない場合はnullptr）</summary>
    Rigidbody* rigidbody = nullptr;

    /// <summary>衝突した相手のGameObject</summary>
    GameObject* gameObject = nullptr;

    /// <summary>衝突した相手のTransform</summary>
    Transform* transform = nullptr;

    /// <summary>衝突点の座標（ワールド座標）</summary>
    VECTOR contactPoint = VGet(0, 0, 0);

    /// <summary>衝突面の法線ベクトル</summary>
    VECTOR contactNormal = VGet(0, 1, 0);

    /// <summary>めり込み深度</summary>
    float penetrationDepth = 0.0f;
};
