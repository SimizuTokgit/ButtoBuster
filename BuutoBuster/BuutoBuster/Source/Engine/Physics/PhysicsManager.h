#pragma once
#include <vector>
#include <unordered_set>
#include <utility>
#include "DxLib.h"
#include "Collision.h"

// 前方宣言
class Collider;
class CapsuleCollider;
class MeshCollider;
class Rigidbody;
class GameObject;

/// <summary>
/// 衝突ペアを管理するための構造体
/// </summary>
struct CollisionPair {
    Collider* a;
    Collider* b;

    bool operator==(const CollisionPair& other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }
};

/// <summary>
/// CollisionPair用のハッシュ関数
/// </summary>
struct CollisionPairHash {
    size_t operator()(const CollisionPair& pair) const {
        // ポインタの順序に依存しないハッシュ
        size_t h1 = std::hash<Collider*>()(pair.a);
        size_t h2 = std::hash<Collider*>()(pair.b);
        return h1 ^ h2;
    }
};

/// <summary>
/// ステージポリゴンの種類
/// </summary>
enum class StageCollisionType {
    Floor,      // 床
    Wall,       // 壁
    Ceiling,    // 天井
    Num
};

/// <summary>
/// 物理演算を管理するシングルトンクラス
/// </summary>
class PhysicsManager {
private:
    /// <summary>キャラクター同士の押し出し力</summary>
    static constexpr float PUSH_POWER = 3.0f;

    // 壁スライドの最大傾斜（これ以上の傾斜は壁として扱う）
    static constexpr float WALL_SLOPE_THRESHOLD = 0.5f;

    // 衝突判定の許容誤差
    static constexpr float COLLISION_EPSILON = 0.01f;

    std::vector<Collider*> _colliders;
    std::vector<Rigidbody*> _rigidbodies;
    std::vector<MeshCollider*> _meshColliders;

    // 前フレームで衝突していたペア（Enter/Stay/Exit判定用）
    std::unordered_set<CollisionPair, CollisionPairHash> _previousCollisions;
    std::unordered_set<CollisionPair, CollisionPairHash> _currentCollisions;

    // MeshColliderとの衝突ペア（Enter/Stay/Exit判定用）
    std::unordered_set<CollisionPair, CollisionPairHash> _previousMeshCollisions;
    std::unordered_set<CollisionPair, CollisionPairHash> _currentMeshCollisions;

public:
    /// <summary>シングルトンインスタンスを取得</summary>
    static PhysicsManager& Instance();

    // コピー禁止
    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;

    /// <summary>Colliderを登録</summary>
    void RegisterCollider(Collider* collider);

    /// <summary>Colliderの登録を解除</summary>
    void UnregisterCollider(Collider* collider);

    /// <summary>Rigidbodyを登録</summary>
    void RegisterRigidbody(Rigidbody* rigidbody);

    /// <summary>Rigidbodyの登録を解除</summary>
    void UnregisterRigidbody(Rigidbody* rigidbody);

    /// <summary>MeshColliderを登録</summary>
    void RegisterMeshCollider(MeshCollider* meshCollider);

    /// <summary>MeshColliderの登録を解除</summary>
    void UnregisterMeshCollider(MeshCollider* meshCollider);

    /// <summary>ステージのコリジョンモデルを設定（後方互換用）</summary>
    void SetStageCollisionModel(int modelHandle);

    /// <summary>最初のMeshColliderのモデルハンドルを取得（後方互換用）</summary>
    int GetStageCollisionModel() const;

    /// <summary>登録されているMeshColliderのリストを取得</summary>
    const std::vector<MeshCollider*>& GetMeshColliders() const { return _meshColliders; }

    /// <summary>
    /// 物理更新（毎フレーム呼び出す）
    /// </summary>
    /// <param name="deltaTime">経過時間（秒）</param>
    void Update(float deltaTime);

    /// <summary>
    /// 指定のカプセルがステージに当たるかチェック
    /// </summary>
    bool CheckCapsuleStageHit(VECTOR pos1, VECTOR pos2, float radius);

    // from から to へ線を引き、地形や小物に当たった一番手前の位置を返す
    // Unity の Physics.Linecast と同じ カメラが岩にめり込まないよう引き寄せるときに使う
    bool Linecast(VECTOR from, VECTOR to, VECTOR& outHitPosition) const;

    /// <summary>
    /// 全登録をクリア（シーン切り替え時など）
    /// </summary>
    void Clear() {
        _colliders.clear();
        _rigidbodies.clear();
        _meshColliders.clear();
        _previousCollisions.clear();
        _currentCollisions.clear();
        _previousMeshCollisions.clear();
        _currentMeshCollisions.clear();
    }

private:
    PhysicsManager() = default;
    ~PhysicsManager() = default;

    /// <summary>全Rigidbodyの物理更新</summary>
    void UpdateRigidbodies(float deltaTime);

    /// <summary>
    /// 指定のColliderを衝突ペアの記録から全て取り除く
    ///
    /// Colliderが破棄されたときに呼ぶ。
    /// 記録に残したままだと、次フレームのEnter/Exit判定で
    /// 解放済みポインタを参照してクラッシュする
    /// </summary>
    void ForgetCollisionPairs(Collider* collider);

    /// <summary>全Collider間の衝突判定</summary>
    void DetectCollisions();

    /// <summary>ステージとの衝突判定・応答</summary>
    void ProcessStageCollisions(float deltaTime);

    /// <summary>
    /// カプセル形状のRigidbodyのステージ衝突処理
    /// </summary>
    void ProcessCapsuleStageCollision(
        Rigidbody* rb,
        CapsuleCollider* capsule,
        float deltaTime
    );

    /// <summary>
    /// 壁スライドベクトルを計算
    /// </summary>
    VECTOR CalculateSlideVector(VECTOR moveVector, VECTOR wallNormal);

    /// <summary>
    /// カプセル同士の衝突判定
    /// </summary>
    bool CheckCapsuleCapsule(
        CapsuleCollider* a,
        CapsuleCollider* b,
        Collision& outCollision
    );

    /// <summary>
    /// 2つのCollider間の押し出し処理
    /// </summary>
    void PushOutColliders(
        Collider* a,
        Collider* b,
        const Collision& collision
    );

    /// <summary>
    /// Enter/Stay/Exitコールバックを発火
    /// </summary>
    void InvokeCollisionCallbacks();

    /// <summary>
    /// MeshColliderとの衝突コールバックを発火
    /// </summary>
    void InvokeMeshCollisionCallbacks();

    /// <summary>
    /// Collision構造体を作成
    /// </summary>
    Collision CreateCollision(
        Collider* other,
        VECTOR contactPoint,
        VECTOR contactNormal,
        float penetration
    );
};
