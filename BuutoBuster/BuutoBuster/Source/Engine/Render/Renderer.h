#pragma once
#include "Component.h"
#include "Transform.h"
#include "RenderManager.h"
#include "DxLib.h"

/// <summary>
/// 描画コンポーネントの基底クラス
/// UnityのRenderer相当（Componentを継承）
/// MeshRenderer, SkinnedMeshRenderer等の基底となる
/// RenderManagerに自動登録され、自動的に描画される
/// </summary>
class Renderer : public Component {
public:
    /// <summary>有効/無効（描画するかどうか）</summary>
    bool enabled = true;

    /// <summary>マテリアルカラー（将来拡張用）</summary>
    COLOR_F color = GetColorF(1.0f, 1.0f, 1.0f, 1.0f);

    /// <summary>影を落とすか</summary>
    bool castShadows = true;

    /// <summary>影を受けるか</summary>
    bool receiveShadows = true;

    /// <summary>3Dモデルハンドル（準定数）</summary>
    int ModelHandle = -1;

    /// <summary>
    /// 描く順番 小さいほど先 Unity の Render Queue と同じ考え方
    /// 半透明のエフェクトは TRANSPARENT にして、不透明なモデルのあとに描く
    /// </summary>
    int renderQueue = RENDER_QUEUE_OPAQUE;

    static constexpr int RENDER_QUEUE_OPAQUE = 0;
    static constexpr int RENDER_QUEUE_TRANSPARENT = 1000;

private:
    /// <summary>RenderManagerに登録済みか</summary>
    bool _isRegistered = false;

public:
    Renderer() = default;

    ~Renderer() override {
        Unregister();
        Unload();
    }

    /// <summary>
    /// モデルを読み込む
    /// 読み込み成功時にRenderManagerに自動登録
    /// </summary>
    /// <param name="modelPath">モデルファイルパス</param>
    /// <returns>読み込み成功ならtrue</returns>
    virtual bool Load(const char* modelPath) {
        Unload();
        ModelHandle = MV1LoadModel(modelPath);
        
        if (ModelHandle != -1) {
            Register();
            return true;
        }
        return false;
    }

    /// <summary>
    /// 既にロード済みのモデルを複製して使う
    ///
    /// 同じ敵を大量に出すときに MV1LoadModel を繰り返すとロードが重いため、
    /// 1体分だけ読み込んでおき、以降は複製して使う。
    /// 複製ハンドルは元とは独立しているので、
    /// 個体ごとに別のポーズを取らせることができ、個別に破棄しても安全。
    /// </summary>
    /// <param name="sourceHandle">複製元のモデルハンドル</param>
    /// <returns>複製成功ならtrue</returns>
    bool LoadDuplicate(int sourceHandle) {
        Unload();

        if (sourceHandle == -1) return false;

        ModelHandle = MV1DuplicateModel(sourceHandle);
        if (ModelHandle == -1) return false;

        Register();
        return true;
    }

    /// <summary>
    /// モデルを解放
    /// </summary>
    virtual void Unload() {
        Unregister();
        if (ModelHandle != -1) {
            MV1DeleteModel(ModelHandle);
            ModelHandle = -1;
        }
    }

    /// <summary>
    /// RenderManagerに登録
    /// </summary>
    void Register() {
        if (!_isRegistered) {
            RenderManager::Instance().Register(this);
            _isRegistered = true;
        }
    }

    /// <summary>
    /// RenderManagerから登録解除
    /// </summary>
    void Unregister() {
        if (_isRegistered) {
            RenderManager::Instance().Unregister(this);
            _isRegistered = false;
        }
    }

    /// <summary>
    /// 描画（派生クラスでオーバーライド）
    /// </summary>
    virtual void Render() = 0;

    /// <summary>
    /// 読み込まれているか
    /// </summary>
    bool IsLoaded() const { return ModelHandle != -1; }

protected:
    /// <summary>
    /// Transformを適用（派生クラスで使用）
    /// Quaternionから回転行列を生成し、位置・スケールと合成して
    /// MV1SetMatrixで一括適用する
    /// </summary>
    void ApplyTransform() {
        if (ModelHandle == -1 || !transform) return;

        VECTOR pos = transform->position;
        Quaternion rot = transform->rotation;
        VECTOR scl = transform->localScale;

        // Quaternionから回転行列を生成
        MATRIX rotMat = rot.ToMatrix();

        // スケール × 回転 × 平行移動 の合成行列を構築
        MATRIX mat = MGetIdent();
        mat.m[0][0] = rotMat.m[0][0] * scl.x;
        mat.m[0][1] = rotMat.m[0][1] * scl.x;
        mat.m[0][2] = rotMat.m[0][2] * scl.x;
        mat.m[1][0] = rotMat.m[1][0] * scl.y;
        mat.m[1][1] = rotMat.m[1][1] * scl.y;
        mat.m[1][2] = rotMat.m[1][2] * scl.y;
        mat.m[2][0] = rotMat.m[2][0] * scl.z;
        mat.m[2][1] = rotMat.m[2][1] * scl.z;
        mat.m[2][2] = rotMat.m[2][2] * scl.z;
        mat.m[3][0] = pos.x;
        mat.m[3][1] = pos.y;
        mat.m[3][2] = pos.z;

        MV1SetMatrix(ModelHandle, mat);
    }
};
