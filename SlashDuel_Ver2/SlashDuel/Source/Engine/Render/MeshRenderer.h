#pragma once
#include "Renderer.h"

/// <summary>
/// 静的メッシュ描画コンポーネント
/// UnityのMeshRenderer相当
/// 地形、建物など変形しないオブジェクトに使用
/// </summary>
class MeshRenderer : public Renderer {
public:
    MeshRenderer() = default;
    ~MeshRenderer() override = default;

    /// <summary>
    /// 描画
    /// </summary>
    void Render() override {
        if (!enabled || ModelHandle == -1 || !transform) return;

        ApplyTransform();
        MV1DrawModel(ModelHandle);
    }
};
