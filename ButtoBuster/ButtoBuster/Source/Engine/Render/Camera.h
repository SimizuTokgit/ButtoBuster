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
    // クリップ面の初期値 Skybox が描いたあとに戻す値もこれ
    static constexpr float DEFAULT_NEAR = 10.0f;
    static constexpr float DEFAULT_FAR = 50000.0f;

    // Transform が無いときのカメラ位置
    static constexpr VECTOR DEFAULT_POSITION = { 0.0f, 0.0f, -100.0f };

    // 注視点が未設定のとき、前方のこの距離を見る
    static constexpr float LOOK_DISTANCE = 100.0f;

    // ライトの初期値 斜め上から照らす白い光と、暗い所を少し持ち上げる環境光
    static constexpr VECTOR DEFAULT_LIGHT_DIRECTION = { 0.5f, -1.0f, 0.5f };
    static constexpr COLOR_F DEFAULT_LIGHT_DIFFUSE = { 1.0f, 1.0f, 1.0f, 1.0f };
    static constexpr COLOR_F DEFAULT_LIGHT_AMBIENT = { 0.3f, 0.3f, 0.3f, 1.0f };

    /// <summary>近クリップ面</summary>
    float nearClipPlane = DEFAULT_NEAR;

    /// <summary>遠クリップ面</summary>
    float farClipPlane = DEFAULT_FAR;

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
        SetupCamera_Perspective(Transform::Deg2Rad(fieldOfView));

        // カメラ位置を取得
        VECTOR cameraPos = transform ? transform->localPosition : DEFAULT_POSITION;

        // 注視点を決定
        VECTOR lookAt;
        if (_hasLookAtTarget) {
            lookAt = _lookAtTarget;
        }
        else if (transform) {
            // 注視点が未設定ならTransformの前方向を使用
            float rotY = Transform::Deg2Rad(transform->localEulerAngles.y);
            VECTOR forward = VGet(sinf(rotY), 0, cosf(rotY));
            lookAt = VAdd(cameraPos, VScale(forward, LOOK_DISTANCE));
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
        VECTOR direction = DEFAULT_LIGHT_DIRECTION,
        COLOR_F diffuse = DEFAULT_LIGHT_DIFFUSE,
        COLOR_F ambient = DEFAULT_LIGHT_AMBIENT
    ) {
        VECTOR lightDir = VNorm(direction);
        SetLightDirection(lightDir);
        SetLightDifColor(diffuse);
        SetLightAmbColor(ambient);
    }
};
