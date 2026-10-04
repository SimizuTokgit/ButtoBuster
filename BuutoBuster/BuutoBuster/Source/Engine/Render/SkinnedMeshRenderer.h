#pragma once
#include "Renderer.h"

// 前方宣言
class Animator;

/// <summary>
/// スキンメッシュ描画コンポーネント
/// UnityのSkinnedMeshRenderer相当
/// キャラクターなどボーンで変形するオブジェクトに使用
/// </summary>
class SkinnedMeshRenderer : public Renderer {
public:
    /// <summary>ボーンのルートフレーム名</summary>
    const char* rootBoneName = "root";

    /// <summary>ルートフレームのインデックス（準定数）</summary>
    int RootFrame = -1;

    /// <summary>関連するAnimator</summary>
    Animator* animator = nullptr;

public:
    SkinnedMeshRenderer() = default;
    ~SkinnedMeshRenderer() override = default;

    /// <summary>
    /// モデルを読み込む
    /// </summary>
    /// <param name="modelPath">モデルファイルパス</param>
    /// <returns>読み込み成功ならtrue</returns>
    bool Load(const char* modelPath) override {
        if (!Renderer::Load(modelPath)) {
            return false;
        }

        // ルートフレームを検索
        SetupRootBone(rootBoneName);

        return true;
    }

    /// <summary>
    /// ルートボーンを設定
    /// </summary>
    /// <param name="boneName">ルートボーン名</param>
    /// <returns>成功ならtrue</returns>
    bool SetupRootBone(const char* boneName) {
        if (ModelHandle == -1) return false;

        RootFrame = MV1SearchFrame(ModelHandle, boneName);
        if (RootFrame != -1) {
            MV1SetFrameUserLocalMatrix(ModelHandle, RootFrame, MGetIdent());
        }

        return RootFrame != -1;
    }

    /// <summary>
    /// 描画
    /// </summary>
    void Render() override {
        if (!enabled || ModelHandle == -1 || !transform) return;

        ApplyTransform();
        MV1DrawModel(ModelHandle);
    }

    /// <summary>
    /// ボーンの数を取得
    /// </summary>
    int GetBoneCount() const {
        if (ModelHandle == -1) return 0;
        return MV1GetFrameNum(ModelHandle);
    }

    /// <summary>
    /// 指定ボーンのワールド行列を取得
    /// </summary>
    MATRIX GetBoneMatrix(int boneIndex) const {
        if (ModelHandle == -1) return MGetIdent();
        return MV1GetFrameLocalWorldMatrix(ModelHandle, boneIndex);
    }

    /// <summary>
    /// 指定ボーンの位置を取得
    /// </summary>
    VECTOR GetBonePosition(int boneIndex) const {
        if (ModelHandle == -1) return VGet(0, 0, 0);
        return MV1GetFramePosition(ModelHandle, boneIndex);
    }

    /// <summary>
    /// 指定ボーンの回転をオイラー角（デグリー）で取得
    /// ワールド行列からスケールを除去し、回転成分を抽出する
    /// </summary>
    Quaternion GetBoneRotation(int boneIndex) const {
        if (ModelHandle == -1) return Quaternion::Identity();
        MATRIX mat = MV1GetFrameLocalWorldMatrix(ModelHandle, boneIndex);
        MATRIX rotMat = ExtractRotationMatrix(mat);
        return Quaternion::FromMatrix(rotMat);
    }

    /// <summary>
    /// 指定ボーンのスケールを取得
    /// </summary>
    VECTOR GetBoneScale(int boneIndex) const {
        if (ModelHandle == -1) return VGet(1, 1, 1);
        MATRIX mat = MV1GetFrameLocalWorldMatrix(ModelHandle, boneIndex);
        return ExtractScale(mat);
    }

    /// <summary>
    /// ボーン名からインデックスを取得
    /// </summary>
    int FindBone(const char* boneName) const {
        if (ModelHandle == -1) return -1;
        return MV1SearchFrame(ModelHandle, boneName);
    }

private:
    /// <summary>
    /// 行列からスケール成分を抽出
    /// </summary>
    static VECTOR ExtractScale(const MATRIX& mat) {
        float sx = VSize(VGet(mat.m[0][0], mat.m[0][1], mat.m[0][2]));
        float sy = VSize(VGet(mat.m[1][0], mat.m[1][1], mat.m[1][2]));
        float sz = VSize(VGet(mat.m[2][0], mat.m[2][1], mat.m[2][2]));
        return VGet(sx, sy, sz);
    }

    /// <summary>
    /// 行列からスケールを除去した純粋な回転行列を抽出
    /// </summary>
    static MATRIX ExtractRotationMatrix(const MATRIX& mat) {
        VECTOR scale = ExtractScale(mat);
        MATRIX rotMat = MGetIdent();
        if (scale.x > 0.0f) { rotMat.m[0][0] = mat.m[0][0] / scale.x; rotMat.m[0][1] = mat.m[0][1] / scale.x; rotMat.m[0][2] = mat.m[0][2] / scale.x; }
        if (scale.y > 0.0f) { rotMat.m[1][0] = mat.m[1][0] / scale.y; rotMat.m[1][1] = mat.m[1][1] / scale.y; rotMat.m[1][2] = mat.m[1][2] / scale.y; }
        if (scale.z > 0.0f) { rotMat.m[2][0] = mat.m[2][0] / scale.z; rotMat.m[2][1] = mat.m[2][1] / scale.z; rotMat.m[2][2] = mat.m[2][2] / scale.z; }
        return rotMat;
    }
};
