#pragma once
#include "Collider.h"
#include "DxLib.h"

// 前方宣言
class Renderer;

/// <summary>
/// メッシュコライダー（地形用コライダー）
/// UnityのMeshCollider相当
/// 3Dモデルのポリゴンを使用した当たり判定
/// </summary>
class MeshCollider : public Collider {
public:
    /// <summary>凸包として扱うか（高速だが凹形状不可）</summary>
    bool convex = false;

    /// <summary>空間分割数X</summary>
    int divisionX = 8;

    /// <summary>空間分割数Y</summary>
    int divisionY = 8;

    /// <summary>空間分割数Z</summary>
    int divisionZ = 8;

    /// <summary>コリジョン用3Dモデルハンドル（準定数）</summary>
    int ModelHandle = -1;

private:
    /// <summary>モデルを自分で読み込んだかどうか</summary>
    bool _ownModel = false;

    /// <summary>PhysicsManagerに登録済みか</summary>
    bool _isRegistered = false;

public:
    MeshCollider() = default;

    ~MeshCollider() override {
        Unregister();
        ReleaseModel();
    }

    /// <summary>
    /// コリジョン用モデルを読み込む（専用コリジョンモデルがある場合）
    /// </summary>
    /// <param name="filePath">モデルファイルパス</param>
    /// <returns>読み込み成功ならtrue</returns>
    bool Load(const char* filePath) {
        ReleaseModel();

        ModelHandle = MV1LoadModel(filePath);
        if (ModelHandle == -1) {
            return false;
        }

        _ownModel = true;
        SetupCollisionInfo();

        return true;
    }

    /// <summary>
    /// 読み込み済みのモデルを複製し、置く場所の行列を当ててから判定を作る
    /// ステージの小物は同じモデルを何十個も置くので、毎回ファイルから読まない
    /// 判定は作った時点の行列で固まるので、動かさないものに使う
    /// </summary>
    bool LoadDuplicate(int sourceHandle, const MATRIX& worldMatrix) {
        ReleaseModel();
        if (sourceHandle == -1) return false;

        ModelHandle = MV1DuplicateModel(sourceHandle);
        if (ModelHandle == -1) return false;

        _ownModel = true;
        MV1SetMatrix(ModelHandle, worldMatrix);
        SetupCollisionInfo();

        // 行列を当てた後の形で判定を作り直す 念のため
        MV1RefreshCollInfo(ModelHandle, -1);
        return true;
    }

    /// <summary>
    /// 既存のモデルハンドルを設定（MeshRendererと共有する場合）
    /// </summary>
    /// <param name="handle">モデルハンドル</param>
    void SetModelHandle(int handle) {
        ReleaseModel();

        ModelHandle = handle;
        _ownModel = false;

        if (ModelHandle != -1) {
            SetupCollisionInfo();
        }
    }

    /// <summary>
    /// Rendererからモデルハンドルを取得して設定
    /// </summary>
    /// <param name="renderer">MeshRenderer/SkinnedMeshRenderer</param>
    void SetFromRenderer(Renderer* renderer);

    /// <summary>
    /// PhysicsManagerに登録
    /// </summary>
    void Register() {
        if (!_isRegistered) {
            PhysicsManager::Instance().RegisterMeshCollider(this);
            _isRegistered = true;
        }
    }

    /// <summary>
    /// PhysicsManagerから登録解除
    /// </summary>
    void Unregister() {
        if (_isRegistered) {
            PhysicsManager::Instance().UnregisterMeshCollider(this);
            _isRegistered = false;
        }
    }

    /// <summary>
    /// コリジョン用モデルをデバッグ描画する
    /// </summary>
    /// <param name="color">描画色</param>
    void DrawGizmo(unsigned int color = 0x00FF00) const {
        if (ModelHandle == -1) return;

        MV1DrawModelDebug(
            ModelHandle,
            color,
            FALSE,
            1.0f,
            TRUE,
            FALSE
        );
    }

    /// <summary>
    /// 読み込まれているか
    /// </summary>
    bool IsLoaded() const { return ModelHandle != -1; }

    /// <summary>
    /// ワールド座標での中心位置を取得（オーバーライド）
    /// </summary>
    VECTOR GetWorldCenter() const override {
        if (transform) {
            return VAdd(transform->position, center);
        }
        return center;
    }

private:
    /// <summary>
    /// コリジョン情報を構築
    /// </summary>
    void SetupCollisionInfo() {
        if (ModelHandle != -1) {
            MV1SetupCollInfo(ModelHandle, -1, divisionX, divisionY, divisionZ);
        }
    }

    /// <summary>
    /// モデルを解放（自分で読み込んだ場合のみ削除）
    /// </summary>
    void ReleaseModel() {
        if (ModelHandle != -1 && _ownModel) {
            MV1DeleteModel(ModelHandle);
        }
        ModelHandle = -1;
        _ownModel = false;
    }
};
