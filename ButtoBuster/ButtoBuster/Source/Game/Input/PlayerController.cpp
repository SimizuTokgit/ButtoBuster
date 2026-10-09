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
    }
    else {
        UpdateViewReset();
        UpdateView();

        input.move = ReadMove();
        input.technique = UpdateCombination(pressed, held);

        // 同時押しを待っている間はガードを出さない
        // ガード + ジャンプの回避を押そうとして、一瞬だけ構えるのを防ぐ
        input.isGuardHeld = (held & GUARD_BIT) != 0 && !_isWaiting;

        // 強攻撃は、攻撃か溜めのボタン (R2) を押し続けている間溜める
        input.isHeavyHeld = (held & (ATTACK_BIT | HEAVY_BIT)) != 0;
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
    // L2 や右クリックを握ったまま攻撃で強斬り、ジャンプで回避
    int combined = _collectedButtons | (held & GUARD_BIT);

    // 回避 溜め 必殺技の専用のボタンは組み合わせを待たない
    // 2つ揃ったときもそれ以上は待たない 先にガードを握っていれば、押した瞬間に揃う
    bool isDedicated = (combined & (DODGE_BIT | HEAVY_BIT | SPECIAL_BIT)) != 0;
    bool isPair = CountBits(combined) >= 2;
    if (!isDedicated && !isPair && _waitedFrames < COMBINE_WAIT_FRAMES) return Technique::None;

    _isWaiting = false;
    return Resolve(combined);
}

int PlayerController::ReadHeldButtons() const {
    const auto& input = InputSystem::Instance();
    int buttons = 0;

    // パッドは X で攻撃、L2 でガード、A でジャンプ、R1 で回避、R2 で溜め、Y で必殺技 (L1 は視点を戻す)
    // キーボードとマウスは攻撃 ガード ジャンプの 3 つで、回避と溜めは組み合わせで出す 必殺技だけは F に分ける
    if (input.KeyHeld(KEY_INPUT_J) || input.MouseHeld(MOUSE_INPUT_LEFT) || input.PadHeld(XINPUT_BUTTON_X)) {
        buttons |= ATTACK_BIT;
    }
    if (input.KeyHeld(KEY_INPUT_K) || input.MouseHeld(MOUSE_INPUT_RIGHT) || input.PadLeftTriggerHeld()) {
        buttons |= GUARD_BIT;
    }
    if (input.KeyHeld(KEY_INPUT_SPACE) || input.PadHeld(XINPUT_BUTTON_A)) {
        buttons |= JUMP_BIT;
    }
    if (input.PadHeld(XINPUT_BUTTON_RIGHT_SHOULDER)) {
        buttons |= DODGE_BIT;
    }
    if (input.PadRightTriggerHeld()) {
        buttons |= HEAVY_BIT;
    }
    if (input.KeyHeld(KEY_INPUT_F) || input.PadHeld(XINPUT_BUTTON_Y)) {
        buttons |= SPECIAL_BIT;
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
    if (!hasInput || isNewDirection || _hasViewInput) {
        _moveForward = _camera->GetGroundForward();
        _moveRight = _camera->GetGroundRight();
        _moveBasisDegree = degree;
        _hasMoveBasis = hasInput;
    }

    return VAdd(VScale(_moveRight, stick.x), VScale(_moveForward, stick.z));
}

void PlayerController::UpdateViewReset() {
    const auto& input = InputSystem::Instance();

    // 視点を背中側へ戻す (正面を向く) C キー、L1
    // 画面の説明は C と L1 だけ書く L ホイール押し R3 も前からの操作として残しておく
    bool isResetPressed = input.PadPressed(XINPUT_BUTTON_LEFT_SHOULDER)
        || input.PadPressed(XINPUT_BUTTON_RIGHT_THUMB)
        || input.KeyPressed(KEY_INPUT_C)
        || input.KeyPressed(KEY_INPUT_L)
        || input.MousePressed(MOUSE_INPUT_MIDDLE);
    if (isResetPressed && _camera) {
        _camera->ResetBehind(_player->GetForward());
    }
}

void PlayerController::UpdateView() {
    const auto& input = InputSystem::Instance();

    // ヒットストップやスローの間も、視点は同じ速さで回したいので実時間で数える
    float deltaTime = Time::UnscaledDeltaTime();
    VECTOR stick = input.RightStick();
    _hasViewInput = false;

    if (!_camera) return;

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

Technique PlayerController::Resolve(int buttons) {
    bool isAttack = (buttons & ATTACK_BIT) != 0;
    bool isGuard = (buttons & GUARD_BIT) != 0;
    bool isJump = (buttons & JUMP_BIT) != 0;
    bool isDodge = (buttons & DODGE_BIT) != 0;
    bool isHeavy = (buttons & HEAVY_BIT) != 0;
    bool isSpecial = (buttons & SPECIAL_BIT) != 0;

    // いくつも同時に押されたら守りを優先する 危ない場面で慌てて全部押しがちなので
    // 必殺技は専用のボタンなので、守りの次に見る
    if (isDodge || (isGuard && isJump)) return Technique::Dodge;
    if (isSpecial) return Technique::Special;
    if (isAttack && isJump) return Technique::AntiAir;
    if (isHeavy || (isAttack && isGuard)) return Technique::StrongSlash;
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
