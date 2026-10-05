#include "PlayerController.h"
#include "Player.h"
#include "CameraFollow.h"
#include "InputSystem.h"
#include "GameObject.h"
#include "Time.h"
#include <cmath>

PlayerController::~PlayerController() {
    // タイトルへ戻ったらカーソルを返す
    InputSystem::Instance().SetMouseLook(false);
}

void PlayerController::Start() {
    _player = GetComponent<Player>();
}

void PlayerController::Update(float deltaTime) {
    if (!_player) return;

    InputInfo input;

    // 戦っている間だけマウスで視点を回す 倒れたらカーソルを返す
    InputSystem::Instance().SetMouseLook(!isInputLocked);

    // 押している印を 1 回だけ読み、押した瞬間はそこから決める
    // 受け付けない間も読んでおく 握ったまま受け付けが戻ったときに、押した瞬間として出ないように
    int held = ReadHeldButtons();
    int pressed = held & ~_prevHeldButtons;
    _prevHeldButtons = held;

    if (isInputLocked) {
        _isWaiting = false;
        _collectedButtons = 0;
        _targetLock.Release();
        if (_camera) _camera->ClearLockPoint();
    }
    else {
        UpdateLockOn();
        UpdateView();

        input.move = ReadMove();
        input.look = GetLockOnDirection();
        input.technique = UpdateCombination(pressed, held);

        // 同時押しを待っている間はガードを出さない
        // ガード + ジャンプの回避を押そうとして、一瞬だけ構えるのを防ぐ
        input.isGuardHeld = (held & GUARD_BIT) != 0 && !_isWaiting;

        // 強攻撃は、攻撃を押し続けている間溜める
        input.isHeavyHeld = (held & ATTACK_BIT) != 0;
    }

    _player->Execute(input, deltaTime);
}

Technique PlayerController::UpdateCombination(int pressed, int held) {
    if (!_isWaiting) {
        if (pressed == 0) return Technique::None;

        _isWaiting = true;
        _waitedFrames = 0;
        _collectedButtons = 0;
    }

    _collectedButtons |= pressed;
    _waitedFrames++;

    // ガードは押しっぱなしで使うので、先に押して握ったままでも組み合わせに入れる
    // RB や右クリックを握ったまま攻撃で強斬り、ジャンプで回避
    int combined = _collectedButtons | (held & GUARD_BIT);

    // 2つ揃ったらそれ以上は待たない 先にガードを握っていれば、押した瞬間に揃う
    bool isPair = CountBits(combined) >= 2;
    if (!isPair && _waitedFrames < COMBINE_WAIT_FRAMES) return Technique::None;

    _isWaiting = false;
    return Resolve(combined);
}

int PlayerController::ReadHeldButtons() const {
    const auto& input = InputSystem::Instance();
    int buttons = 0;

    // パッドは RT で攻撃、RB でガード、A でジャンプ 強攻撃はキーボードと同じく攻撃 + ガード
    if (input.KeyHeld(KEY_INPUT_J) || input.MouseHeld(MOUSE_INPUT_LEFT) || input.PadRightTriggerHeld()) {
        buttons |= ATTACK_BIT;
    }
    if (input.KeyHeld(KEY_INPUT_K) || input.MouseHeld(MOUSE_INPUT_RIGHT) || input.PadHeld(XINPUT_BUTTON_RIGHT_SHOULDER)) {
        buttons |= GUARD_BIT;
    }
    if (input.KeyHeld(KEY_INPUT_SPACE) || input.PadHeld(XINPUT_BUTTON_A)) {
        buttons |= JUMP_BIT;
    }
    return buttons;
}

VECTOR PlayerController::ReadMove() {
    const auto& input = InputSystem::Instance();
    VECTOR stick = input.LeftStick();

    // x が画面の右 z が画面の奥
    if (input.KeyHeld(KEY_INPUT_W) || input.KeyHeld(KEY_INPUT_UP)) stick.z += 1.0f;
    if (input.KeyHeld(KEY_INPUT_S) || input.KeyHeld(KEY_INPUT_DOWN)) stick.z -= 1.0f;
    if (input.KeyHeld(KEY_INPUT_D) || input.KeyHeld(KEY_INPUT_RIGHT)) stick.x += 1.0f;
    if (input.KeyHeld(KEY_INPUT_A) || input.KeyHeld(KEY_INPUT_LEFT)) stick.x -= 1.0f;

    float length = VSize(stick);
    if (length > 1.0f) stick = VScale(stick, 1.0f / length);

    if (!_camera) return stick;

    // 画面の奥へ倒したら、カメラの向いている先へ進む
    // ただし押し続けている間は、押し始めたときのカメラの向きを使い続ける
    // 背中へ回り込むカメラに合わせて向きを変えると、横を押しただけで円を描いてしまう
    bool hasInput = VSize(stick) > 0.1f;
    float degree = atan2f(stick.x, stick.z) * 180.0f / DX_PI_F;
    float change = degree - _moveBasisDegree;
    while (change > 180.0f) change -= 360.0f;
    while (change < -180.0f) change += 360.0f;

    bool isNewDirection = !_hasMoveBasis || fabsf(change) > MOVE_REBASE_DEGREE;
    if (!hasInput || isNewDirection || _hasViewInput || _targetLock.HasTarget()) {
        _moveForward = _camera->GetGroundForward();
        _moveRight = _camera->GetGroundRight();
        _moveBasisDegree = degree;
        _hasMoveBasis = hasInput;
    }

    return VAdd(VScale(_moveRight, stick.x), VScale(_moveForward, stick.z));
}

void PlayerController::UpdateLockOn() {
    const auto& input = InputSystem::Instance();

    // 倒した相手や、遠くへ離れた相手からは外す
    _targetLock.Validate(*_player);

    bool isLockPressed = input.KeyPressed(KEY_INPUT_L)
        || input.MousePressed(MOUSE_INPUT_MIDDLE)
        || input.PadLeftTriggerPressed();

    if (isLockPressed) {
        if (_targetLock.HasTarget()) {
            _targetLock.Release();
        }
        else if (!_targetLock.Acquire(*_player, GetViewForward())) {
            // 狙える敵がいなければ、視点を背中側へ戻す 無双の ZL と同じ
            if (_camera) _camera->ResetBehind(_player->GetForward());
        }
    }

    // 右スティックの押し込みは、ロックオンしていなければいつでも背中側へ戻す
    bool isResetPressed = input.PadPressed(XINPUT_BUTTON_RIGHT_THUMB);
    if (isResetPressed && !_targetLock.HasTarget() && _camera) {
        _camera->ResetBehind(_player->GetForward());
    }
}

void PlayerController::UpdateView() {
    const auto& input = InputSystem::Instance();

    // ヒットストップやスローの間も、視点は同じ速さで回したいので実時間で数える
    float deltaTime = Time::UnscaledDeltaTime();
    VECTOR stick = input.RightStick();
    _hasViewInput = false;

    if (_targetLock.HasTarget()) {
        // ロックオン中は視点が相手を追うので、回す操作は隣の敵へ移す合図にする
        int direction = 0;

        // 倒した瞬間だけ移す 倒したままにしても次々に移らないように
        bool isFlicked = fabsf(stick.x) > FLICK_THRESHOLD;
        if (isFlicked && !_wasStickFlicked) direction = (stick.x > 0.0f) ? 1 : -1;
        _wasStickFlicked = isFlicked;

        if (input.KeyPressed(KEY_INPUT_E)) direction = 1;
        if (input.KeyPressed(KEY_INPUT_Q)) direction = -1;

        _mouseSwitchAmount = _mouseSwitchAmount * expf(-MOUSE_SWITCH_DECAY * deltaTime) + input.MouseDeltaX();
        if (fabsf(_mouseSwitchAmount) > MOUSE_SWITCH_DISTANCE) {
            direction = (_mouseSwitchAmount > 0.0f) ? 1 : -1;
            _mouseSwitchAmount = 0.0f;
        }

        if (direction != 0) _targetLock.Switch(*_player, direction);

        if (_camera) _camera->SetLockPoint(_targetLock.GetTarget()->GetCenter());
        return;
    }

    // ロックオンした瞬間に倒していたスティックで、すぐ隣へ移らないように覚えておく
    _wasStickFlicked = fabsf(stick.x) > FLICK_THRESHOLD;
    _mouseSwitchAmount = 0.0f;

    if (!_camera) return;
    _camera->ClearLockPoint();

    float keyTurn = 0.0f;
    if (input.KeyHeld(KEY_INPUT_E)) keyTurn += 1.0f;
    if (input.KeyHeld(KEY_INPUT_Q)) keyTurn -= 1.0f;

    float yaw = (stick.x * STICK_YAW_SPEED + keyTurn * KEY_YAW_SPEED) * deltaTime
        + input.MouseDeltaX() * MOUSE_SENSITIVITY;

    // スティックを上へ倒すと見上げる マウスも上へ動かすと見上げる
    float pitch = -stick.z * STICK_PITCH_SPEED * deltaTime
        + input.MouseDeltaY() * MOUSE_SENSITIVITY;

    _hasViewInput = fabsf(yaw) > 0.01f;
    _camera->Rotate(yaw, pitch);
}

VECTOR PlayerController::GetViewForward() const {
    if (_camera) return _camera->GetGroundForward();
    return _player->GetForward();
}

VECTOR PlayerController::GetLockOnDirection() const {
    const Character* target = _targetLock.GetTarget();
    if (!target) return VGet(0.0f, 0.0f, 0.0f);

    VECTOR toTarget = VSub(target->GetPosition(), _player->GetPosition());
    toTarget.y = 0.0f;
    return toTarget;
}

Technique PlayerController::Resolve(int buttons) {
    bool isAttack = (buttons & ATTACK_BIT) != 0;
    bool isGuard = (buttons & GUARD_BIT) != 0;
    bool isJump = (buttons & JUMP_BIT) != 0;

    // 3つ同時は守りを優先する 危ない場面で慌てて全部押しがちなので
    if (isGuard && isJump) return Technique::Dodge;
    if (isAttack && isJump) return Technique::AntiAir;
    if (isAttack && isGuard) return Technique::StrongSlash;
    if (isAttack) return Technique::Slash;
    if (isJump) return Technique::Jump;

    // ガードだけは技ではなく、押している間ずっと構える
    return Technique::None;
}

int PlayerController::CountBits(int bits) {
    int count = 0;
    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count;
}
