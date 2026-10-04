#pragma once
#include "Component.h"
#include "Transform.h"
#include "DxLib.h"

// 前方宣言
class Skybox;

/// <summary>
/// カメラコンポーネント
/// UnityのCamera相当
/// </summary>
class Camera : public Component {
public:
    /// <summary>近クリップ面</summary>
    float nearClipPlane = 10.0f;

    /// <summary>遠クリップ面</summary>
    float farClipPlane = 50000.0f;

    /// <summary>視野角（度）</summary>
    float fieldOfView = 60.0f;

    /// <summary>背景色（Skyboxがない場合に使用）</summary>
    unsigned int backgroundColor = 0x87CEEB;  // スカイブルー

private:
    /// <summary>関連するSkyboxコンポーネント</summary>
    Skybox* _skybox = nullptr;

    /// <summary>メインカメラかどうか</summary>
    bool _isMainCamera = false;

    /// <summary>注視点（外部スクリプトから設定される）</summary>
    VECTOR _lookAtTarget = VGet(0, 0, 0);

    /// <summary>注視点が設定されているか</summary>
    bool _hasLookAtTarget = false;

    /// <summary>メインカメラへの静的参照</summary>
    static Camera*& GetMainCameraRef() {
        static Camera* mainCamera = nullptr;
        return mainCamera;
    }

public:
    Camera() = default;

    ~Camera() override {
        if (GetMainCameraRef() == this) {
            GetMainCameraRef() = nullptr;
        }
    }

    /// <summary>
    /// メインカメラとして設定
    /// </summary>
    void SetAsMainCamera() {
        GetMainCameraRef() = this;
        _isMainCamera = true;
    }

    /// <summary>
    /// メインカメラを取得
    /// </summary>
    static Camera* GetMain() { return GetMainCameraRef(); }

    /// <summary>
    /// Skyboxを設定
    /// </summary>
    void SetSkybox(Skybox* skybox) { _skybox = skybox; }

    /// <summary>
    /// Skyboxを取得
    /// </summary>
    Skybox* GetSkybox() const { return _skybox; }

    /// <summary>
    /// 注視点を設定（CameraFollow等の外部スクリプトから呼ぶ）
    /// </summary>
    void SetLookAtTarget(VECTOR target) {
        _lookAtTarget = target;
        _hasLookAtTarget = true;
    }

    /// <summary>
    /// カメラ設定を適用
    /// </summary>
    void Apply() {
        // クリップ面を設定
        SetCameraNearFar(nearClipPlane, farClipPlane);

        // 視野角を設定（ラジアンに変換）
        SetupCamera_Perspective(fieldOfView * DX_PI_F / 180.0f);

        // カメラ位置を取得
        VECTOR cameraPos = transform ? transform->localPosition : VGet(0, 0, -100);

        // 注視点を決定
        VECTOR lookAt;
        if (_hasLookAtTarget) {
            lookAt = _lookAtTarget;
        }
        else if (transform) {
            // 注視点が未設定ならTransformの前方向を使用
            float rotY = Transform::Deg2Rad(transform->localEulerAngles.y);
            VECTOR forward = VGet(sinf(rotY), 0, cosf(rotY));
            lookAt = VAdd(cameraPos, VScale(forward, 100.0f));
        }
        else {
            lookAt = VGet(0, 0, 0);
        }

        SetCameraPositionAndTarget_UpVecY(cameraPos, lookAt);
    }

    /// <summary>
    /// カメラ位置を取得
    /// </summary>
    VECTOR GetPosition() const {
        if (transform) {
            return transform->localPosition;
        }
        return VGet(0, 0, 0);
    }

    /// <summary>
    /// ライトをセットアップ
    /// </summary>
    /// <param name="direction">ライト方向</param>
    /// <param name="diffuse">ディフューズカラー</param>
    /// <param name="ambient">アンビエントカラー</param>
    void SetupDirectionalLight(
        VECTOR direction = VGet(0.5f, -1.0f, 0.5f),
        COLOR_F diffuse = GetColorF(1.0f, 1.0f, 1.0f, 1.0f),
        COLOR_F ambient = GetColorF(0.3f, 0.3f, 0.3f, 1.0f)
    ) {
        VECTOR lightDir = VNorm(direction);
        SetLightDirection(lightDir);
        SetLightDifColor(diffuse);
        SetLightAmbColor(ambient);
    }
};
