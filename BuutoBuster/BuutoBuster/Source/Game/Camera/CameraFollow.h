#pragma once
#include "MonoBehaviour.h"
#include "DxLib.h"

class Transform;
class Camera;

class CameraFollow : public MonoBehaviour {
public:
    Transform* target = nullptr;

    // 足元からこの高さを見る 肩の高さにして、背中越しに前を見る
    float lookHeight = 150.0f;

    // 見ている点からカメラまでの距離 近いほど背中に張り付いた TPS らしい見え方になる
    float distance = 430.0f;

    // 見る点を右へずらす量 プレイヤーが画面の少し左に来て、右肩越しに前が見える
    // ロックオン中は相手を真ん中に置きたいので、ずらすのをやめる
    float shoulderOffset = 55.0f;

    // 大きいほどすぐ追いつく 上下はゆっくりにして、跳んだときに画面が揺れすぎないようにする
    float followSharpness = 10.0f;
    float verticalSharpness = 5.0f;

    // 動いている間、背中側へ回り込む強さ 大きいほど早く背中に付く 0 なら回り込まない
    // TPS のように、走る向きがいつも画面の奥になる
    float behindFollowSharpness = 2.5f;

    // 回り込むいちばん速い速さ 度/秒 速すぎると画面が振り回されて酔う
    float behindFollowMaxSpeed = 150.0f;

private:
    // 見下ろす角度 度 上を向きすぎると地面にめり込み、下を向きすぎると周りが見えない
    static constexpr float BASE_PITCH = 10.0f;
    static constexpr float MIN_PITCH = -10.0f;
    static constexpr float MAX_PITCH = 55.0f;

    Camera* _camera = nullptr;
    VECTOR _focus = VGet(0.0f, 0.0f, 0.0f);
    bool _hasFocus = false;

    // 追いかける相手の前のフレームの位置と、そこから求めた動きの速さ
    VECTOR _prevGoal = VGet(0.0f, 0.0f, 0.0f);
    VECTOR _moveVelocity = VGet(0.0f, 0.0f, 0.0f);

    // 向き 度 左右は +Z が 0 で右回りがプラス 上下は見下ろすほどプラス
    float _yaw = 0.0f;
    float _pitch = BASE_PITCH;

    // Rotate で受け取って、次の Update でまとめて回す
    float _pendingYaw = 0.0f;
    float _pendingPitch = 0.0f;

    // 最後に手で回してからの時間 回した直後は自動で回り込まない
    float _idleTime = 0.0f;

    bool _isResetting = false;
    float _resetYaw = 0.0f;

    // ロックオンの相手 外した後も少しの間だけ使い、注視点をなめらかに戻す
    bool _hasLockPoint = false;
    VECTOR _lockPoint = VGet(0.0f, 0.0f, 0.0f);
    float _lockBlend = 0.0f;

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

    // 背中側へ回す ロックオンする相手がいないときに ZL を押したのと同じ
    void ResetBehind(VECTOR facing);

    // この点がプレイヤーの向こうに来るよう回り続ける ロックオンの相手の胴体を毎フレーム渡す
    void SetLockPoint(VECTOR point);
    void ClearLockPoint();

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
    float GetDesiredDistance() const;
    float ResolveObstacle(VECTOR lookAt, VECTOR back, float desiredDistance, float deltaTime);
    VECTOR GetShakeOffset();
    void UpdateZoomPunch();
};
