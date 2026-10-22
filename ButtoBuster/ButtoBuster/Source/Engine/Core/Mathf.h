#pragma once
#include "DxLib.h"
#include <cmath>

// よく使う計算 Unity の Mathf と Random.Range にあたる
// 同じ計算をファイルごとに書かないよう、ここに 1 つだけ置く
// std::min と std::max は Windows.h のマクロとぶつかるので、ここでは使わない
namespace Mathf {
    // 乱数の細かさ 0〜1 をこの数に分けて引く
    constexpr int RANDOM_STEPS = 10000;

    inline float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    inline float Clamp01(float value) {
        return Clamp(value, 0.0f, 1.0f);
    }

    // 角度を -180〜180 度にそろえる 2 つの向きの差を、近いほうの回り方で測るのに使う
    inline float WrapDegree(float degree) {
        while (degree > 180.0f) degree -= 360.0f;
        while (degree < -180.0f) degree += 360.0f;
        return degree;
    }

    // 角度を -π〜π にそろえる
    inline float WrapRadian(float radian) {
        while (radian > DX_PI_F) radian -= DX_TWO_PI_F;
        while (radian < -DX_PI_F) radian += DX_TWO_PI_F;
        return radian;
    }

    // 地面に沿った向きを、Y 軸まわりの角度 (度) にする +Z が 0 +X が 90
    inline float YawFromDirection(VECTOR direction) {
        return atan2f(direction.x, direction.z) * 180.0f / DX_PI_F;
    }

    // 0〜1 の t を、最初に勢いよく進み だんだんゆっくりになる 0〜1 にする
    inline float EaseOutQuad(float t) {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    // min〜max のどこか
    inline float RandomRange(float min, float max) {
        return min + (max - min) * (static_cast<float>(GetRand(RANDOM_STEPS)) / RANDOM_STEPS);
    }

    // 0〜1 の割合を 0〜255 にする 音量や透明度を DxLib に渡すときに使う 範囲の外は端にそろえる
    inline int ToByte(float rate) {
        return static_cast<int>(Clamp01(rate) * 255.0f);
    }
}
