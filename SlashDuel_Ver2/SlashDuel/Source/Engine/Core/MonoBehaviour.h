#pragma once
#include "Behaviour.h"

// 前方宣言
struct Collision;
class Collider;

/// <summary>
/// UnityのMonoBehaviour相当
/// ゲームロジックを記述するコンポーネントの基底クラス
/// </summary>
class MonoBehaviour : public Behaviour {
public:
    MonoBehaviour() = default;
    virtual ~MonoBehaviour() = default;

public:
    /// <summary>初期化時に一度だけ呼ばれる</summary>
    virtual void Start() {}

    /// <summary>毎フレーム呼ばれる</summary>
    virtual void Update(float deltaTime) {}

    // ----- コリジョンイベント（物理衝突） -----
    // 引数: Collision（衝突情報）

    /// <summary>衝突開始時に呼ばれる</summary>
    virtual void OnCollisionEnter(const Collision& collision) {}

    /// <summary>衝突継続中に毎フレーム呼ばれる</summary>
    virtual void OnCollisionStay(const Collision& collision) {}

    /// <summary>衝突終了時に呼ばれる</summary>
    virtual void OnCollisionExit(const Collision& collision) {}

    // ----- トリガーイベント（すり抜け衝突） -----
    // 引数: Collider*（接触した相手のコライダー）

    /// <summary>トリガー開始時に呼ばれる</summary>
    virtual void OnTriggerEnter(Collider* other) {}

    /// <summary>トリガー中に居る間、毎フレーム呼ばれる</summary>
    virtual void OnTriggerStay(Collider* other) {}

    /// <summary>トリガーから出た時に呼ばれる</summary>
    virtual void OnTriggerExit(Collider* other) {}

    // ----- ギズモイベント -----

    /// <summary>デバッグ描画用（レンダリング後に呼ばれる）</summary>
    virtual void OnDrawGizmos() {}
};
