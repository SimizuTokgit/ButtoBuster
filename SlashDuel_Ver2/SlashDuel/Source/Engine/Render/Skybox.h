#pragma once
#include "Component.h"
#include "DxLib.h"

/// <summary>
/// スカイボックス（空）コンポーネント
/// UnityのSkybox相当
/// カメラに追従して空を描画
/// </summary>
class Skybox : public Component {
public:
    /// <summary>スカイボックス用の遠クリップ面</summary>
    float farClipPlane = 100000.0f;

    /// <summary>スカイボックス用の近クリップ面</summary>
    float nearClipPlane = 10.0f;

    /// <summary>空用3Dモデルハンドル（準定数）</summary>
    int ModelHandle = -1;

private:
    /// <summary>追従するカメラ位置（毎フレーム更新）</summary>
    VECTOR _cameraPosition = VGet(0, 0, 0);

public:
    Skybox() = default;

    ~Skybox() override {
        Unload();
    }

    /// <summary>
    /// スカイボックスモデルを読み込む
    /// </summary>
    /// <param name="filePath">モデルファイルパス</param>
    /// <returns>読み込み成功ならtrue</returns>
    bool Load(const char* filePath) {
        Unload();

        ModelHandle = MV1LoadModel(filePath);
        return ModelHandle != -1;
    }

    /// <summary>
    /// モデルを解放
    /// </summary>
    void Unload() {
        if (ModelHandle != -1) {
            MV1DeleteModel(ModelHandle);
            ModelHandle = -1;
        }
    }

    /// <summary>
    /// カメラ位置を設定（描画前に呼ぶ）
    /// </summary>
    void SetCameraPosition(VECTOR position) {
        _cameraPosition = position;
    }

    /// <summary>
    /// スカイボックスを描画
    /// 他のオブジェクトより先に描画すること
    /// </summary>
    /// <param name="savedNear">復元用の近クリップ面</param>
    /// <param name="savedFar">復元用の遠クリップ面</param>
    void Render(float savedNear = 10.0f, float savedFar = 50000.0f) {
        if (ModelHandle == -1) return;

        SetCameraNearFar(nearClipPlane, farClipPlane);
        MV1SetPosition(ModelHandle, _cameraPosition);

        MV1SetUseZBuffer(ModelHandle, FALSE);
        MV1SetWriteZBuffer(ModelHandle, FALSE);

        MV1DrawModel(ModelHandle);

        MV1SetUseZBuffer(ModelHandle, TRUE);
        MV1SetWriteZBuffer(ModelHandle, TRUE);

        SetCameraNearFar(savedNear, savedFar);
    }

    /// <summary>
    /// 読み込まれているか
    /// </summary>
    bool IsLoaded() const { return ModelHandle != -1; }
};
