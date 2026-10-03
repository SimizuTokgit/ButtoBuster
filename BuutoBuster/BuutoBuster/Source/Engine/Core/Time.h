#pragma once

// ゲーム内の時間
// ヒットストップやスロー再生は timeScale を変えて作る
// ゲームの処理は DeltaTime 演出の時間計測は UnscaledDeltaTime を使う
class Time {
private:
    static inline float _deltaTime = 0.0f;
    static inline float _unscaledDeltaTime = 0.0f;
    static inline float _timeScale = 1.0f;

public:
    // 倍率を掛けた後の経過時間 (s)
    static float DeltaTime() { return _deltaTime; }

    // 実時間の経過時間 (s)
    static float UnscaledDeltaTime() { return _unscaledDeltaTime; }

    static float GetTimeScale() { return _timeScale; }

    // 0 で停止 1 で等速
    static void SetTimeScale(float scale) {
        if (scale < 0.0f) scale = 0.0f;
        _timeScale = scale;
    }

    // System から毎フレーム呼ぶ
    static void Update(float unscaledDeltaTime) {
        _unscaledDeltaTime = unscaledDeltaTime;
        _deltaTime = unscaledDeltaTime * _timeScale;
    }
};
