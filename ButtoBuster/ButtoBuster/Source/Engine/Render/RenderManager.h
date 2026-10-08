#pragma once
#include <vector>
#include <algorithm>

// 前方宣言
class Renderer;

/// <summary>
/// Renderer管理シングルトンクラス
/// 登録されたRendererを自動的に描画する
/// </summary>
class RenderManager {
private:
    std::vector<Renderer*> _renderers;

public:
    /// <summary>シングルトンインスタンスを取得</summary>
    static RenderManager& Instance() {
        static RenderManager instance;
        return instance;
    }

    // コピー禁止
    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;

    /// <summary>
    /// Rendererを登録
    /// </summary>
    void Register(Renderer* renderer) {
        if (renderer) {
            auto it = std::find(_renderers.begin(), _renderers.end(), renderer);
            if (it == _renderers.end()) {
                _renderers.push_back(renderer);
            }
        }
    }

    /// <summary>
    /// Rendererの登録を解除
    /// </summary>
    void Unregister(Renderer* renderer) {
        auto it = std::find(_renderers.begin(), _renderers.end(), renderer);
        if (it != _renderers.end()) {
            _renderers.erase(it);
        }
    }

    /// <summary>
    /// 登録されている全Rendererを描画
    /// </summary>
    void RenderAll();

    /// <summary>
    /// 登録されているRenderer数を取得
    /// </summary>
    size_t GetRendererCount() const { return _renderers.size(); }

    /// <summary>
    /// 全Rendererをクリア（シーン切り替え時など）
    /// </summary>
    void Clear() { _renderers.clear(); }

private:
    RenderManager() = default;
    ~RenderManager() = default;
};
