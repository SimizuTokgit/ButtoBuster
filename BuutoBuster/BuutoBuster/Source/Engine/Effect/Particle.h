#pragma once
#include "DxLib.h"

/// <summary>
/// パーティクル放出形状
/// </summary>
enum class EmissionShape {
    Point,   // Transformの位置から放出
    Sphere   // 球体範囲内からランダムに放出
};

/// <summary>
/// パーティクル1粒の情報
/// </summary>
struct Particle {
    /// <summary>ワールド座標</summary>
    VECTOR position = VGet(0, 0, 0);

    /// <summary>移動速度ベクトル</summary>
    VECTOR velocity = VGet(0, 0, 0);

    /// <summary>残りライフタイム（秒）</summary>
    float remainingLifetime = 0.0f;

    /// <summary>初期ライフタイム（秒）</summary>
    float startLifetime = 0.0f;

    /// <summary>初期サイズ</summary>
    float startSize = 1.0f;

    /// <summary>不透明度（0.0〜1.0）</summary>
    float alpha = 1.0f;

    /// <summary>有効かどうか</summary>
    bool alive = false;
};
