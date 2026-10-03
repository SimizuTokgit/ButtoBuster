#pragma once
#include "InputSystem.h"
#include "Time.h"
#include "DxLib.h"
#include <functional>
#include <string>
#include <utility>
#include <vector>

// 制作用のデバッグ表示とチート
// F1 で一覧の表示を切り替える
// ゲーム側から AddToggle / AddCommand で項目を足す
class DebugMenu {
private:
    // 押すたびに ON/OFF が入れ替わる項目
    struct Toggle {
        int key;
        std::string name;
        bool value;
    };

    // 押した瞬間に1回だけ走る項目
    struct Command {
        int key;
        std::string name;
        std::function<void()> action;
    };

    static constexpr int MENU_KEY = KEY_INPUT_F1;
    static constexpr int LINE_HEIGHT = 18;

    std::vector<Toggle> _toggles;
    std::vector<Command> _commands;
    bool _visible = false;

    // FPS計測
    float _fpsTimer = 0.0f;
    int _frameCount = 0;
    int _fps = 0;

public:
    static DebugMenu& Instance() {
        static DebugMenu instance;
        return instance;
    }

    DebugMenu(const DebugMenu&) = delete;
    DebugMenu& operator=(const DebugMenu&) = delete;

    void AddToggle(int key, const std::string& name, bool defaultValue = false) {
        // 同じ名前は二重に登録しない
        for (const auto& toggle : _toggles) {
            if (toggle.name == name) return;
        }
        _toggles.push_back(Toggle{ key, name, defaultValue });
    }

    void AddCommand(int key, const std::string& name, std::function<void()> action) {
        _commands.push_back(Command{ key, name, std::move(action) });
    }

    bool GetToggle(const std::string& name) const {
        for (const auto& toggle : _toggles) {
            if (toggle.name == name) return toggle.value;
        }
        return false;
    }

    void SetToggle(const std::string& name, bool value) {
        for (auto& toggle : _toggles) {
            if (toggle.name == name) {
                toggle.value = value;
                return;
            }
        }
    }

    bool IsVisible() const { return _visible; }

    // メインループから毎フレーム呼ぶ
    void Update() {
        const auto& input = InputSystem::Instance();

        if (input.KeyPressed(MENU_KEY)) _visible = !_visible;

        for (auto& toggle : _toggles) {
            if (input.KeyPressed(toggle.key)) toggle.value = !toggle.value;
        }

        for (auto& command : _commands) {
            if (input.KeyPressed(command.key) && command.action) command.action();
        }

        UpdateFps();
    }

    void Render() {
        if (!_visible) return;

        int y = 10;

        DrawFormatString(10, y, 0xFFFF00, "F1 デバッグ表示   FPS %d", _fps);
        y += LINE_HEIGHT;

        for (const auto& toggle : _toggles) {
            unsigned int color = toggle.value ? 0x00FF88 : 0x888888;
            DrawFormatString(10, y, color, "%-4s %s  %s",
                KeyName(toggle.key), toggle.name.c_str(), toggle.value ? "ON" : "OFF");
            y += LINE_HEIGHT;
        }

        for (const auto& command : _commands) {
            DrawFormatString(10, y, 0xFFFFFF, "%-4s %s", KeyName(command.key), command.name.c_str());
            y += LINE_HEIGHT;
        }
    }

    // シーンを切り替えるときに呼ぶ
    // コマンドがシーン内のオブジェクトを掴んだまま残るのを防ぐ
    void Clear() {
        _commands.clear();

        // トグルは残す 当たり判定表示などはシーンをまたいで使うため
    }

private:
    DebugMenu() = default;
    ~DebugMenu() = default;

    void UpdateFps() {
        _frameCount++;
        _fpsTimer += Time::UnscaledDeltaTime();

        if (_fpsTimer < 1.0f) return;

        _fps = _frameCount;
        _frameCount = 0;
        _fpsTimer -= 1.0f;
    }

    // DxLib のキーコードは連番ではないので個別に返す
    static const char* KeyName(int key) {
        switch (key) {
        case KEY_INPUT_F1:  return "F1";
        case KEY_INPUT_F2:  return "F2";
        case KEY_INPUT_F3:  return "F3";
        case KEY_INPUT_F4:  return "F4";
        case KEY_INPUT_F5:  return "F5";
        case KEY_INPUT_F6:  return "F6";
        case KEY_INPUT_F7:  return "F7";
        case KEY_INPUT_F8:  return "F8";
        case KEY_INPUT_F9:  return "F9";
        case KEY_INPUT_F10: return "F10";
        case KEY_INPUT_F11: return "F11";
        case KEY_INPUT_F12: return "F12";
        default:            return "";
        }
    }
};
