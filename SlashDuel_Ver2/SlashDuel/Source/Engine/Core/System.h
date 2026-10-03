#pragma once
#include <functional>

// メインループ
// 最初のシーンは WinMain から渡す エンジン側はシーンの種類を知らない
class System {
private:
    static constexpr int GAME_SCREEN_WIDTH = 1280;
    static constexpr int GAME_SCREEN_HEIGHT = 720;

    // deltaTime の上限 (1/60秒)
    // 処理落ちや非アクティブ復帰で一気に時間が進むのを防ぐ
    static constexpr float MAX_DELTA_TIME = 1.0f / 60.0f;

    int _prevTime = 0;
    bool _quitRequested = false;

public:
    bool Main(const std::function<bool()>& loadFirstScene);

private:
    bool Initialize(const std::function<bool()>& loadFirstScene);
    void MainLoop();
    void Terminate();
};
