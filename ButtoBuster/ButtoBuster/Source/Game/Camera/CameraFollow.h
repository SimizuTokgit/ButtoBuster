#pragma once
#include "MonoBehaviour.h"
#include "CameraData.h"
#include "DxLib.h"

class Transform;
class Camera;

// 追いかけ方の数値は CameraData にまとめてある
class CameraFollow : public MonoBehaviour {
public:
    Transform* target = nullptr;

    // 追いかけ方の数値 追いかける処理はここから読む
    CameraData data;

private:
    Camera* _camera = nullptr;
    VECTOR _focus = VGet(0.0f, 0.0f, 0.0f);
    bool _hasFocus = false;

    // 追いかける相手の前のフレームの位置と、そこから求めた動きの速さ
    VECTOR _prevGoal = VGet(0.0f, 0.0f, 0.0f);
    VECTOR _moveVelocity = VGet(0.0f, 0.0f, 0.0f);

    // 向き 度 左右は +Z が 0 で右回りがプラス 上下は見下ろすほどプラス
    // 上下は使う前に SnapToTarget で data.basePitch にする
    float _yaw = 0.0f;
    float _pitch = 0.0f;

    // Rotate で受け取って、次の Update でまとめて回す
    float _pendingYaw = 0.0f;
    float _pendingPitch = 0.0f;

    // 最後に手で回してからの時間 回した直後は自動で回り込まない
    float _idleTime = 0.0f;

    bool _isResetting = false;
    float _resetYaw = 0.0f;

    // 障害物で縮めた今の距離
    float _currentDistance = 0.0f;

    float _shakePower = 0.0f;
    float _shakeTime = 0.0f;
    float _shakeDuration = 0.0f;

    float _baseFieldOfView = 60.0f;
    float _punchDegrees = 0.0f;
    float _punchTime = 0.0f;
    float _punchDuration = 0.0f;

public:
    void Start() override;
    void Update(float deltaTime) override;

    // 手で回す 度 右回りと見下ろす向きがプラス
    void Rotate(float yawDegrees, float pitchDegrees);

    // 背中側へ回す 視点を戻すボタンで使う
    void ResetBehind(VECTOR facing);

    // 地面に沿った前と右 移動の入力を、カメラから見た向きに直すのに使う
    VECTOR GetGroundForward() const;
    VECTOR GetGroundRight() const;

    // 強さは揺れ幅の最大 長さは秒
    // 強いほうが優先される
    void Shake(float power, float duration);

    // 一瞬だけ画角を狭めて寄り、ゆっくり戻す 重い一撃や最後の1体を倒したときに使う
    // 強いほうが優先される
    void ZoomPunch(float degrees, float duration);

    // すぐに目標の位置へ移し、背中側から見る シーン開始やリスタートで使う
    void SnapToTarget();

private:
    void UpdateFocus(float deltaTime);
    void UpdateAngles(float deltaTime);
    VECTOR GetLookAt() const;
    float ResolveObstacle(VECTOR lookAt, VECTOR back, float desiredDistance, float deltaTime);
    VECTOR GetShakeOffset();
    void UpdateZoomPunch();
};
