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

    // トリガーは 0 から 255 半分より深く引いたら押した、4 分の 1 より戻したら離したことにする
    // 押すと離すの間を空けておくと、半分あたりで指が止まっても、押した離したが細かく繰り返されない
    static constexpr int TRIGGER_PRESS = 128;
    static constexpr int TRIGGER_RELEASE = 64;

    char _keys[256] = {};
    char _prevKeys[256] = {};

    XINPUT_STATE _pad = {};
    XINPUT_STATE _prevPad = {};
    bool _padConnected = false;

    // トリガーを押しているか 押した離したの深さが違うので、値からではなくここに覚えておく
    bool _leftTrigger = false;
    bool _prevLeftTrigger = false;
    bool _rightTrigger = false;
    bool _prevRightTrigger = false;

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
        _prevLeftTrigger = _leftTrigger;
        _prevRightTrigger = _rightTrigger;

        // 非アクティブの間は何も押していない扱いにする
        if (!GetActiveFlag()) {
            memset(_keys, 0, sizeof(_keys));
            memset(&_pad, 0, sizeof(_pad));
            _mouse = 0;
            _padConnected = false;
            _leftTrigger = false;
            _rightTrigger = false;

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

        _leftTrigger = IsTriggerHeld(_leftTrigger, _pad.LeftTrigger);
        _rightTrigger = IsTriggerHeld(_rightTrigger, _pad.RightTrigger);

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

    // 左のトリガー ガードに当てる
    bool PadLeftTriggerHeld() const { return _leftTrigger; }
    bool PadLeftTriggerPressed() const { return _leftTrigger && !_prevLeftTrigger; }

    // 右のトリガー 溜め斬りに当てる
    bool PadRightTriggerHeld() const { return _rightTrigger; }
    bool PadRightTriggerPressed() const { return _rightTrigger && !_prevRightTrigger; }

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

    // 戻る モード選択からタイトルへ戻るのに使う
    bool CancelPressed() const {
        return KeyPressed(KEY_INPUT_BACK)
            || MousePressed(MOUSE_INPUT_RIGHT)
            || PadPressed(XINPUT_BUTTON_B);
    }

    // メニューの選択を上下に動かす キー 十字キー 左スティック
    // スティックは、遊びの外へ倒した瞬間の 1 回だけ数える
    bool MenuUpPressed() const {
        return KeyPressed(KEY_INPUT_W) || KeyPressed(KEY_INPUT_UP)
            || PadPressed(XINPUT_BUTTON_DPAD_UP)
            || (StickY(_pad) > DEAD_ZONE && StickY(_prevPad) <= DEAD_ZONE);
    }

    bool MenuDownPressed() const {
        return KeyPressed(KEY_INPUT_S) || KeyPressed(KEY_INPUT_DOWN)
            || PadPressed(XINPUT_BUTTON_DPAD_DOWN)
            || (StickY(_pad) < -DEAD_ZONE && StickY(_prevPad) >= -DEAD_ZONE);
    }

    // マウスのカーソルの位置 画面の座標
    void GetMousePosition(int* x, int* y) const {
        GetMousePoint(x, y);
    }

    // スキップ チュートリアルを飛ばすのに使う 長押しで決まるので、押している間を返す
    bool SkipHeld() const {
        return KeyHeld(KEY_INPUT_RETURN)
            || KeyHeld(KEY_INPUT_NUMPADENTER)
            || PadHeld(XINPUT_BUTTON_START);
    }

private:
    InputSystem() = default;
    ~InputSystem() = default;

    // 左スティックの上下 上がプラス -1〜1
    float StickY(const XINPUT_STATE& pad) const {
        if (!_padConnected) return 0.0f;
        return pad.ThumbLY / STICK_MAX;
    }

    // 押していなければ TRIGGER_PRESS まで引いたら押した、押していれば TRIGGER_RELEASE より戻したら離した
    static bool IsTriggerHeld(bool wasHeld, int value) {
        return wasHeld ? (value >= TRIGGER_RELEASE) : (value >= TRIGGER_PRESS);
    }

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
