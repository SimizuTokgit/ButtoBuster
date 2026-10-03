#pragma once
#include "MonoBehaviour.h"
#include "SkinnedMeshRenderer.h"
#include "Quaternion.h"
#include "DxLib.h"

/// <summary>
/// ボーン追従コンポーネント
/// 指定したSkinnedMeshRendererのボーンのワールド行列を
/// 毎フレーム取得し、自身のTransformに反映する
/// 
/// 用途: Playerモデルの手ボーン("wp")に武器を追従させる等
/// </summary>
class BoneFollower : public MonoBehaviour {
private:
    /// <summary>追従対象のSkinnedMeshRenderer</summary>
    SkinnedMeshRenderer* _targetRenderer = nullptr;

    /// <summary>追従するボーンのインデックス</summary>
    int _boneIndex = -1;

public:
    BoneFollower() = default;
    ~BoneFollower() override = default;

    /// <summary>
    /// 追従対象を設定する
    /// </summary>
    /// <param name="renderer">追従対象のSkinnedMeshRenderer</param>
    /// <param name="boneName">追従するボーン名</param>
    /// <returns>設定成功ならtrue</returns>
    bool Setup(SkinnedMeshRenderer* renderer, const char* boneName) {
        if (!renderer) return false;

        _targetRenderer = renderer;
        _boneIndex = renderer->FindBone(boneName);

        if (_boneIndex != -1) {
            // ワールド座標計算で親の変換を無視する
            // BoneFollowerはボーンのワールド行列を直接localに書き込むため、
            // 親Transformの変換が二重適用されるのを防ぐ
            // （Transform親子関係は維持されるのでStart/Updateの走査に影響しない）
            transform->ignoreParentTransform = true;
        }

        return _boneIndex != -1;
    }

    /// <summary>
    /// 毎フレーム、ボーンのワールド行列を取得してTransformに反映
    /// </summary>
    void Update(float deltaTime) override {
        if (!_targetRenderer || _boneIndex == -1) return;

        transform->localPosition = _targetRenderer->GetBonePosition(_boneIndex);
        transform->localRotation = _targetRenderer->GetBoneRotation(_boneIndex);
        transform->localScale = _targetRenderer->GetBoneScale(_boneIndex);
    }

    /// <summary>
    /// セットアップ済みかどうか
    /// </summary>
    bool IsValid() const { return _targetRenderer != nullptr && _boneIndex != -1; }
};
