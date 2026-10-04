#pragma once
#include "DxLib.h"
#include <cmath>
#include <cstring>

// 入力の窓口
// 毎フレーム1回だけハードを読み 他はここから受け取る
// DxLib の入力関数を直接触るのはこのクラスだけにする
class InputSystem {
private:
    static constexpr float STICK_MAX = 32767.0f;

    // スティックの遊び 20%までは倒していない扱い
    static constexpr float DEAD_ZONE = 0.2f;

    // トリガーは 0 から 255 半分より深く引いたら押したことにする
    static constexpr int TRIGGER_THRESHOLD = 128;

    char _keys[256] = {};
    char _prevKeys[256] = {};

    XINPUT_STATE _pad = {};
    XINPUT_STATE _prevPad = {};
    bool _padConnected = false;

    int _mouse = 0;
    int _prevMouse = 0;

    // マウスで視点を回すときは、カーソルを隠して毎フレーム画面の真ん中へ戻す
    // 戻した位置からの動きが、そのフレームの移動量になる
    bool _isMouseLook = false;
    bool _hasMouseAnchor = false;
    int _lastMouseX = 0;
    int _lastMouseY = 0;
    float _mouseDeltaX = 0.0f;
    float _mouseDeltaY = 0.0f;

public:
    static InputSystem& Instance() {
        static InputSystem instance;
        return instance;
    }

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    // メインループの先頭で1回だけ呼ぶ
    void Update() {
        memcpy(_prevKeys, _keys, sizeof(_keys));
        _prevPad = _pad;
        _prevMouse = _mouse;

        // 非アクティブの間は何も押していない扱いにする
        if (!GetActiveFlag()) {
            memset(_keys, 0, sizeof(_keys));
            memset(&_pad, 0, sizeof(_pad));
            _mouse = 0;
            _padConnected = false;

            // ほかのウィンドウを触っている間はカーソルを戻さない
            // 戻ってきた最初のフレームは、離れていた間の動きを拾わないよう捨てる
            _mouseDeltaX = 0.0f;
            _mouseDeltaY = 0.0f;
            _hasMouseAnchor = false;
            return;
        }

        GetHitKeyStateAll(_keys);

        _padConnected = (GetJoypadXInputState(DX_INPUT_PAD1, &_pad) == 0);
        if (!_padConnected) memset(&_pad, 0, sizeof(_pad));

        _mouse = GetMouseInput();

        UpdateMouseLook();
    }

    // ----- キーボード -----

    bool KeyHeld(int key) const {
        if (key < 0 || key >= 256) return false;
        return _keys[key] != 0;
    }

    bool KeyPressed(int key) const {
        if (key < 0 || key >= 256) return false;
        return _keys[key] != 0 && _prevKeys[key] == 0;
    }

    // ----- ゲームパッド -----

    bool PadConnected() const { return _padConnected; }

    bool PadHeld(int button) const { return _pad.Buttons[button] != 0; }

    bool PadPressed(int button) const {
        return _pad.Buttons[button] != 0 && _prevPad.Buttons[button] == 0;
    }

    // 左のトリガー 無双のロックオン (ZL) に当てる
    bool PadLeftTriggerHeld() const { return _pad.LeftTrigger >= TRIGGER_THRESHOLD; }

    bool PadLeftTriggerPressed() const {
        return _pad.LeftTrigger >= TRIGGER_THRESHOLD && _prevPad.LeftTrigger < TRIGGER_THRESHOLD;
    }

    // ----- マウス -----

    bool MouseHeld(int button) const { return (_mouse & button) != 0; }

    bool MousePressed(int button) const {
        return (_mouse & button) != 0 && (_prevMouse & button) == 0;
    }

    // 視点をマウスで回すかどうか 戦っている間だけ入れる
    // 入れている間はカーソルが消えて、ウィンドウの外へ出なくなる
    void SetMouseLook(bool isEnabled) {
        if (isEnabled == _isMouseLook) return;

        _isMouseLook = isEnabled;
        _hasMouseAnchor = false;
        _mouseDeltaX = 0.0f;
        _mouseDeltaY = 0.0f;
        SetMouseDispFlag(isEnabled ? FALSE : TRUE);
    }

    // 前のフレームからのマウスの移動量 画面の右と下がプラス 視点を回していないときは 0
    float MouseDeltaX() const { return _mouseDeltaX; }
    float MouseDeltaY() const { return _mouseDeltaY; }

    // ----- まとめて使うもの -----

    // 左スティック 遊びの内側はゼロにする
    VECTOR LeftStick() const {
        if (!_padConnected) return VGet(0.0f, 0.0f, 0.0f);

        float x = _pad.ThumbLX / STICK_MAX;
        float z = _pad.ThumbLY / STICK_MAX;

        if (fabsf(x) < DEAD_ZONE) x = 0.0f;
        if (fabsf(z) < DEAD_ZONE) z = 0.0f;

        return VGet(x, 0.0f, z);
    }

    // 右スティック 左と同じく x が右 z が上
    VECTOR RightStick() const {
        if (!_padConnected) return VGet(0.0f, 0.0f, 0.0f);

        float x = _pad.ThumbRX / STICK_MAX;
        float z = _pad.ThumbRY / STICK_MAX;

        if (fabsf(x) < DEAD_ZONE) x = 0.0f;
        if (fabsf(z) < DEAD_ZONE) z = 0.0f;

        return VGet(x, 0.0f, z);
    }

    // 決定 タイトルやリザルトの進行に使う
    bool ConfirmHeld() const {
        return KeyHeld(KEY_INPUT_SPACE)
            || MouseHeld(MOUSE_INPUT_LEFT)
            || PadHeld(XINPUT_BUTTON_A);
    }

    bool ConfirmPressed() const {
        return KeyPressed(KEY_INPUT_SPACE)
            || MousePressed(MOUSE_INPUT_LEFT)
            || PadPressed(XINPUT_BUTTON_A);
    }

private:
    InputSystem() = default;
    ~InputSystem() = default;

    void UpdateMouseLook() {
        _mouseDeltaX = 0.0f;
        _mouseDeltaY = 0.0f;
        if (!_isMouseLook) return;

        int width = 0;
        int height = 0;
        GetDrawScreenSize(&width, &height);
        int centerX = width / 2;
        int centerY = height / 2;

        int x = 0;
        int y = 0;
        GetMousePoint(&x, &y);

        // 前のフレームの最後に読んだ位置との差を取る
        // 視点を回し始めた最初の 1 回は、それまでのカーソルの位置からの差になるので捨てる
        if (_hasMouseAnchor) {
            _mouseDeltaX = static_cast<float>(x - _lastMouseX);
            _mouseDeltaY = static_cast<float>(y - _lastMouseY);
        }

        // 真ん中へ戻して、戻った先を読み直しておく
        // 戻せなかった環境でも、次のフレームの差は実際に動いた分になる
        SetMousePoint(centerX, centerY);
        GetMousePoint(&_lastMouseX, &_lastMouseY);
        _hasMouseAnchor = true;
    }
};
