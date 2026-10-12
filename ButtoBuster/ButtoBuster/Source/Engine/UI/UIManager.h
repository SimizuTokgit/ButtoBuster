#pragma once
#include <vector>
#include "ContainerUtil.h"

// 前方宣言
class UIImage;

/// <summary>
/// UI要素の管理シングルトン
/// 登録されたUIImageをsortingOrder順に描画する
/// </summary>
class UIManager {
private:
    std::vector<UIImage*> _images;

public:
    /// <summary>シングルトンインスタンスを取得</summary>
    static UIManager& Instance() {
        static UIManager instance;
        return instance;
    }

    // コピー禁止
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    /// <summary>UIImageを登録</summary>
    void Register(UIImage* image) {
        ContainerUtil::AddUnique(_images, image);
    }

    /// <summary>UIImageの登録解除</summary>
    void Unregister(UIImage* image) {
        ContainerUtil::Remove(_images, image);
    }

    /// <summary>登録されている全UIImageを描画</summary>
    void RenderAll();

    /// <summary>登録されているUIImage数を取得</summary>
    size_t GetUIImageCount() const { return _images.size(); }

    /// <summary>全UIImageをクリア（シーン切り替え時など）</summary>
    void Clear() { _images.clear(); }

private:
    UIManager() = default;
    ~UIManager() = default;
};
