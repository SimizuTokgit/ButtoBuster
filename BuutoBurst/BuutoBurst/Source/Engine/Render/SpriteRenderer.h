#pragma once
#include "Renderer.h"

/// <summary>
/// 3D空間にビルボード画像を描画するコンポーネント
/// UnityのSpriteRenderer相当
/// DrawBillboard3Dを使用し、常にカメラに向く
/// </summary>
class SpriteRenderer : public Renderer {
private:
    int _graphHandle = -1;

public:
    /// <summary>ビルボードのサイズ</summary>
    float size = 100.0f;

    /// <summary>Zバッファの読み込みを使用するか</summary>
    bool useZBufferRead = true;

    /// <summary>Zバッファへの書き込みを行うか</summary>
    bool useZBufferWrite = false;

    /// <summary>アルファ値（0-255）</summary>
    int alpha = 255;

    SpriteRenderer() = default;

    ~SpriteRenderer() override {
        if (_graphHandle != -1) {
            DeleteGraph(_graphHandle);
            _graphHandle = -1;
        }
    }

    /// <summary>
    /// 画像ファイルを読み込み、RenderManagerに登録する
    /// </summary>
    /// <param name="imagePath">画像ファイルパス</param>
    /// <returns>読み込み成功ならtrue</returns>
    bool LoadSprite(const char* imagePath) {
        if (_graphHandle != -1) {
            DeleteGraph(_graphHandle);
            _graphHandle = -1;
        }
        _graphHandle = LoadGraph(imagePath);
        if (_graphHandle != -1) {
            Register();
            return true;
        }
        return false;
    }

    /// <summary>画像がロード済みか</summary>
    bool IsSpriteLoaded() const { return _graphHandle != -1; }

    /// <summary>
    /// ビルボード描画（RenderManagerから呼ばれる）
    /// </summary>
    void Render() override {
        if (!enabled || _graphHandle == -1 || !transform) return;

        SetUseZBufferFlag(useZBufferRead ? TRUE : FALSE);
        SetWriteZBufferFlag(useZBufferWrite ? TRUE : FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        DrawBillboard3D(
            transform->position,
            0.5f, 0.5f,
            size,
            0.0f,
            _graphHandle,
            TRUE
        );

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetUseZBufferFlag(TRUE);
        SetWriteZBufferFlag(TRUE);
    }
};
