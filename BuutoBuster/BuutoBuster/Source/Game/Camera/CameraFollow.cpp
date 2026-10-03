#include "CameraFollow.h"
#include "GameObject.h"
#include "Transform.h"
#include "Camera.h"
#include "PhysicsManager.h"
#include "Time.h"
#include <cmath>

namespace {
    // 寄るのにかける割合 残りでゆっくり戻す
    constexpr float PUNCH_IN_RATE = 0.15f;

    // 手で回してから、背中側へ回り込み始めるまで 回した向きをすぐ戻されないように
    constexpr float FOLLOW_DELAY = 0.6f;

    // この速さより遅いときは回り込まない 立ち止まって向きを変えただけで回らないように
    constexpr float FOLLOW_MIN_SPEED = 150.0f;

    // 背中とカメラの向きがこれより離れていたら回り込まない
    // カメラへ向かって走ってくるときに、画面がぐるりと半周するのを防ぐ
    constexpr float FOLLOW_MAX_ANGLE = 150.0f;

    // ロックオンの相手へ向き直る速さ
    constexpr float LOCK_TURN_SHARPNESS = 6.0f;

    // 注視点を相手の方へどれだけ寄せるか 0 でプレイヤー 1 で相手
    constexpr float LOCK_LOOK_WEIGHT = 0.3f;

    // 相手が遠いほど引いて、プレイヤーと相手の両方を画面に入れる
    constexpr float LOCK_EXTRA_DISTANCE_RATE = 0.15f;
    constexpr float LOCK_EXTRA_DISTANCE_MAX = 250.0f;

    // 相手がこれより近いと向きが定まらず回り続けるので、向き直らない
    constexpr float LOCK_MIN_HORIZONTAL = 120.0f;

    // ロックオンを付けたり外したりしたとき、注視点が飛ばないよう混ぜる速さ
    constexpr float LOCK_BLEND_SHARPNESS = 6.0f;

    // 背中側へ戻す速さ
    constexpr float RESET_SHARPNESS = 12.0f;

    // 障害物からどれだけ手前に止めるか 近すぎると近クリップで壁が欠けて見える
    constexpr float OBSTACLE_MARGIN = 30.0f;
    constexpr float MIN_DISTANCE = 120.0f;

    // 遮るものが無くなったとき、元の距離へ戻る速さ 急に戻ると画面が跳ねる
    constexpr float DISTANCE_RETURN_SHARPNESS = 3.0f;

    float ToRadian(float degree) { return degree * DX_PI_F / 180.0f; }

    float WrapDegree(float degree) {
        while (degree > 180.0f) degree -= 360.0f;
        while (degree < -180.0f) degree += 360.0f;
        return degree;
    }

    // 地面に沿った向きを角度にする +Z が 0 +X が 90
    float YawFromDirection(VECTOR direction) {
        return atan2f(direction.x, direction.z) * 180.0f / DX_PI_F;
    }

    // 1 フレームで目標へ近づける割合 フレームレートが変わっても同じ速さになるよう指数で減らす
    float BlendRate(float sharpness, float deltaTime) {
        return 1.0f - expf(-sharpness * deltaTime);
    }
}

void CameraFollow::Start() {
    _camera = GetComponent<Camera>();
    if (_camera) _baseFieldOfView = _camera->fieldOfView;
    SnapToTarget();
}

void CameraFollow::Update(float deltaTime) {
    if (!target || !transform) return;

    if (!_hasFocus) SnapToTarget();

    // ヒットストップやスローの間も、視点を回す手触りは変えたくないので実時間で動かす
    float unscaled = Time::UnscaledDeltaTime();

    UpdateFocus(unscaled);
    UpdateAngles(unscaled);

    // 肩越しに見るよう、見る点もカメラも右へずらす
    float shoulder = shoulderOffset * (1.0f - _lockBlend);
    VECTOR lookAt = VAdd(GetLookAt(), VScale(GetGroundRight(), shoulder));

    // 見ている点からカメラへ向かう向き
    float yaw = ToRadian(_yaw);
    float pitch = ToRadian(_pitch);
    VECTOR back = VGet(-sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(yaw) * cosf(pitch));

    float cameraDistance = ResolveObstacle(lookAt, back, GetDesiredDistance(), unscaled);

    VECTOR shake = GetShakeOffset();
    transform->localPosition = VAdd(VAdd(lookAt, VScale(back, cameraDistance)), shake);

    if (_camera) {
        _camera->SetLookAtTarget(VAdd(lookAt, shake));
    }

    UpdateZoomPunch();
}

void CameraFollow::Rotate(float yawDegrees, float pitchDegrees) {
    _pendingYaw += yawDegrees;
    _pendingPitch += pitchDegrees;
}

void CameraFollow::ResetBehind(VECTOR facing) {
    facing.y = 0.0f;
    if (VSquareSize(facing) < 0.0001f) return;

    _resetYaw = YawFromDirection(facing);
    _isResetting = true;
}

void CameraFollow::SetLockPoint(VECTOR point) {
    _lockPoint = point;
    _hasLockPoint = true;
    _isResetting = false;
}

void CameraFollow::ClearLockPoint() {
    _hasLockPoint = false;
}

VECTOR CameraFollow::GetGroundForward() const {
    float yaw = ToRadian(_yaw);
    return VGet(sinf(yaw), 0.0f, cosf(yaw));
}

VECTOR CameraFollow::GetGroundRight() const {
    float yaw = ToRadian(_yaw);
    return VGet(cosf(yaw), 0.0f, -sinf(yaw));
}

void CameraFollow::Shake(float power, float duration) {
    if (power <= 0.0f || duration <= 0.0f) return;

    float currentPower = (_shakeDuration > 0.0f) ? _shakePower * (_shakeTime / _shakeDuration) : 0.0f;
    if (power < currentPower) return;

    _shakePower = power;
    _shakeTime = duration;
    _shakeDuration = duration;
}

void CameraFollow::ZoomPunch(float degrees, float duration) {
    if (degrees <= 0.0f || duration <= 0.0f) return;
    if (_punchTime > 0.0f && degrees < _punchDegrees) return;

    _punchDegrees = degrees;
    _punchTime = duration;
    _punchDuration = duration;
}

void CameraFollow::SnapToTarget() {
    if (!target) return;

    _focus = target->position;
    _prevGoal = _focus;
    _moveVelocity = VGet(0.0f, 0.0f, 0.0f);
    _hasFocus = true;

    // 背中側から見る
    VECTOR facing = target->forward;
    facing.y = 0.0f;
    if (VSquareSize(facing) > 0.0001f) _yaw = YawFromDirection(facing);
    _pitch = BASE_PITCH;
    _isResetting = false;

    // 次の Update で障害物を見て決め直す
    _currentDistance = 0.0f;
}

void CameraFollow::UpdateFocus(float deltaTime) {
    VECTOR goal = target->position;

    // 走っている向きと速さ 背中側へ回り込むときに使う
    if (deltaTime > 0.0f) {
        VECTOR moved = VSub(goal, _prevGoal);
        moved.y = 0.0f;
        _moveVelocity = VScale(moved, 1.0f / deltaTime);
    }
    _prevGoal = goal;

    float horizontal = BlendRate(followSharpness, deltaTime);
    float vertical = BlendRate(verticalSharpness, deltaTime);
    _focus.x += (goal.x - _focus.x) * horizontal;
    _focus.z += (goal.z - _focus.z) * horizontal;
    _focus.y += (goal.y - _focus.y) * vertical;
}

void CameraFollow::UpdateAngles(float deltaTime) {
    bool hasManualInput = _pendingYaw != 0.0f || _pendingPitch != 0.0f;

    _yaw += _pendingYaw;
    _pitch += _pendingPitch;
    _pendingYaw = 0.0f;
    _pendingPitch = 0.0f;

    if (hasManualInput) {
        _idleTime = 0.0f;
        _isResetting = false;
    }
    else {
        _idleTime += deltaTime;
    }

    _lockBlend += ((_hasLockPoint ? 1.0f : 0.0f) - _lockBlend) * BlendRate(LOCK_BLEND_SHARPNESS, deltaTime);

    if (_hasLockPoint) {
        // プレイヤーから相手への向きの後ろに回る 相手がいつも画面の奥に来る
        VECTOR toLock = VSub(_lockPoint, _focus);
        toLock.y = 0.0f;
        if (VSize(toLock) > LOCK_MIN_HORIZONTAL) {
            float difference = WrapDegree(YawFromDirection(toLock) - _yaw);
            _yaw += difference * BlendRate(LOCK_TURN_SHARPNESS, deltaTime);
        }
    }
    else if (_isResetting) {
        float rate = BlendRate(RESET_SHARPNESS, deltaTime);
        float difference = WrapDegree(_resetYaw - _yaw);
        _yaw += difference * rate;
        _pitch += (BASE_PITCH - _pitch) * rate;
        if (fabsf(difference) < 0.5f) _isResetting = false;
    }
    else if (_idleTime > FOLLOW_DELAY && behindFollowSharpness > 0.0f) {
        // 動いている間は、プレイヤーの背中の向きへ回り込む
        VECTOR facing = target->forward;
        facing.y = 0.0f;
        bool isMoving = VSize(_moveVelocity) > FOLLOW_MIN_SPEED;

        if (isMoving && VSquareSize(facing) > 0.0001f) {
            float difference = WrapDegree(YawFromDirection(facing) - _yaw);
            if (fabsf(difference) < FOLLOW_MAX_ANGLE) {
                float step = difference * BlendRate(behindFollowSharpness, deltaTime);
                float maxStep = behindFollowMaxSpeed * deltaTime;
                if (step > maxStep) step = maxStep;
                if (step < -maxStep) step = -maxStep;
                _yaw += step;
            }
        }
    }

    _yaw = WrapDegree(_yaw);
    if (_pitch < MIN_PITCH) _pitch = MIN_PITCH;
    if (_pitch > MAX_PITCH) _pitch = MAX_PITCH;
}

VECTOR CameraFollow::GetLookAt() const {
    VECTOR lookAt = VAdd(_focus, VGet(0.0f, lookHeight, 0.0f));
    if (_lockBlend <= 0.001f) return lookAt;

    // 相手の方へ少し寄せて、プレイヤーと相手の両方を画面に入れる
    VECTOR toLock = VSub(_lockPoint, lookAt);
    return VAdd(lookAt, VScale(toLock, LOCK_LOOK_WEIGHT * _lockBlend));
}

float CameraFollow::GetDesiredDistance() const {
    if (_lockBlend <= 0.001f) return distance;

    VECTOR toLock = VSub(_lockPoint, _focus);
    toLock.y = 0.0f;
    float extra = VSize(toLock) * LOCK_EXTRA_DISTANCE_RATE;
    if (extra > LOCK_EXTRA_DISTANCE_MAX) extra = LOCK_EXTRA_DISTANCE_MAX;
    return distance + extra * _lockBlend;
}

float CameraFollow::ResolveObstacle(VECTOR lookAt, VECTOR back, float desiredDistance, float deltaTime) {
    float allowed = desiredDistance;

    // 余白の分だけ先まで調べる 壁のすぐ手前に置いて、近クリップで壁が欠けないように
    VECTOR farEnd = VAdd(lookAt, VScale(back, desiredDistance + OBSTACLE_MARGIN));
    VECTOR hit;
    if (PhysicsManager::Instance().Linecast(lookAt, farEnd, hit)) {
        allowed = VSize(VSub(hit, lookAt)) - OBSTACLE_MARGIN;
        if (allowed < MIN_DISTANCE) allowed = MIN_DISTANCE;
    }

    // 遮られたらすぐ寄る 遅れるとその間だけ岩の中が見えてしまう
    // 遮るものが無くなったら、ゆっくり元の距離へ戻す
    if (_currentDistance <= 0.0f || allowed < _currentDistance) {
        _currentDistance = allowed;
    }
    else {
        _currentDistance += (allowed - _currentDistance) * BlendRate(DISTANCE_RETURN_SHARPNESS, deltaTime);
    }
    return _currentDistance;
}

void CameraFollow::UpdateZoomPunch() {
    if (!_camera) return;

    if (_punchTime <= 0.0f) {
        _camera->fieldOfView = _baseFieldOfView;
        return;
    }

    // 揺れと同じく、ヒットストップ中も進める
    _punchTime -= Time::UnscaledDeltaTime();
    if (_punchTime < 0.0f) _punchTime = 0.0f;

    // 0 から 1 へ進む 最初の少しで一気に寄り、残りで戻る
    float progress = 1.0f - _punchTime / _punchDuration;
    float amount = 0.0f;
    if (progress < PUNCH_IN_RATE) {
        amount = progress / PUNCH_IN_RATE;
    }
    else {
        float back = (progress - PUNCH_IN_RATE) / (1.0f - PUNCH_IN_RATE);
        amount = (1.0f - back) * (1.0f - back);
    }

    _camera->fieldOfView = _baseFieldOfView - _punchDegrees * amount;
}

VECTOR CameraFollow::GetShakeOffset() {
    if (_shakeTime <= 0.0f) return VGet(0.0f, 0.0f, 0.0f);

    _shakeTime -= Time::UnscaledDeltaTime();
    if (_shakeTime <= 0.0f) {
        _shakeTime = 0.0f;
        return VGet(0.0f, 0.0f, 0.0f);
    }

    // 終わりに向けて弱める
    float power = _shakePower * (_shakeTime / _shakeDuration);
    float x = (GetRand(2000) / 1000.0f - 1.0f) * power;
    float y = (GetRand(2000) / 1000.0f - 1.0f) * power;

    // カメラが回っても画面の上で同じ揺れ方になるよう、カメラから見た左右と上下で揺らす
    return VAdd(VScale(GetGroundRight(), x), VGet(0.0f, y, 0.0f));
}
