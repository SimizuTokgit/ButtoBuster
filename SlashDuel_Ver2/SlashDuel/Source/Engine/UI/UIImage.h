#pragma once
#include "MonoBehaviour.h"
#include "UIManager.h"
#include "DxLib.h"

/// <summary>
/// スクリーンスペースに画像を描画するUIコンポーネント
/// UnityのCanvas + Image相当
/// UIManagerに自動登録され、sortingOrder順に描画される
/// </summary>
class UIImage : public MonoBehaviour {
public:
    /// <summary>描画順序（小さいほど先に描画）</summary>
    int sortingOrder = 0;

    /// <summary>表示するかどうか</summary>
    bool visible = true;

    /// <summary>スクリーン座標X</summary>
    int screenX = 0;

    /// <summary>スクリーン座標Y</summary>
    int screenY = 0;

    /// <summary>描画幅</summary>
    int width = 0;

    /// <summary>描画高さ</summary>
    int height = 0;

    /// <summary>アルファ値（0-255）</summary>
    int alpha = 255;

    UIImage() = default;

    ~UIImage() override {
        Unregister();
        UnloadImage();
    }

    void Start() override {
        Register();
    }

    /// <summary>
    /// 画像ファイルを読み込む
    /// width/heightが0の場合、画像サイズを自動取得する
    /// </summary>
    /// <param name="imagePath">画像ファイルパス</param>
    /// <returns>読み込み成功ならtrue</returns>
    bool LoadImage(const char* imagePath) {
        UnloadImage();
        _graphHandle = ::LoadGraph(imagePath);
        if (_graphHandle != -1) {
            if (width == 0 || height == 0) {
                GetGraphSize(_graphHandle, &width, &height);
            }
            return true;
        }
        return false;
    }

    /// <summary>画像を解放する</summary>
    void UnloadImage() {
        if (_graphHandle != -1) {
            DeleteGraph(_graphHandle);
            _graphHandle = -1;
        }
    }

    /// <summary>
    /// 描画（UIManagerから呼ばれる）
    /// サブクラスでオーバーライド可能
    /// </summary>
    virtual void Render() {
        if (_graphHandle == -1) return;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawExtendGraph(screenX, screenY, screenX + width, screenY + height, _graphHandle, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    /// <summary>画像がロード済みか</summary>
    bool IsImageLoaded() const { return _graphHandle != -1; }

    /// <summary>画像ハンドルを取得</summary>
    int GetGraphHandle() const { return _graphHandle; }

protected:
    int _graphHandle = -1;

private:
    bool _registered = false;

    void Register() {
        if (!_registered) {
            UIManager::Instance().Register(this);
            _registered = true;
        }
    }

    void Unregister() {
        if (_registered) {
            UIManager::Instance().Unregister(this);
            _registered = false;
        }
    }
};
