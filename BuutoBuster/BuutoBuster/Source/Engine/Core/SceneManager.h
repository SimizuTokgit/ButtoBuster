#pragma once
#include "SceneBase.h"
#include <memory>
#include <functional>

/// <summary>
/// シーン管理シングルトンクラス
/// UnityのSceneManager相当
/// シーンの読み込み・切り替えを管理する
/// </summary>
class SceneManager {
private:
    std::unique_ptr<SceneBase> _currentScene;
    std::function<std::unique_ptr<SceneBase>()> _pendingSceneFactory;

public:
    /// <summary>シングルトンインスタンスを取得</summary>
    static SceneManager& Instance() {
        static SceneManager instance;
        return instance;
    }

    // コピー禁止
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    /// <summary>
    /// シーンを即座にロード（初回ロードや安全なタイミングで使用）
    /// </summary>
    template<typename T>
    bool LoadScene() {
        return LoadSceneInternal(std::make_unique<T>());
    }

    /// <summary>
    /// シーン遷移をリクエスト（Update中から呼ぶ用）
    /// 実際の遷移はフレーム末尾のProcessSceneTransitionで行われる
    /// </summary>
    template<typename T>
    void RequestLoadScene() {
        _pendingSceneFactory = []() -> std::unique_ptr<SceneBase> {
            return std::make_unique<T>();
        };
    }

    /// <summary>
    /// フレーム末尾でシーン遷移を実行
    /// MainLoopのScreenFlip後に毎フレーム呼ぶ
    /// </summary>
    void ProcessSceneTransition();

    /// <summary>現在のシーンを取得</summary>
    SceneBase* GetCurrentScene() const { return _currentScene.get(); }

    /// <summary>シーン遷移リクエストがあるか</summary>
    bool HasPendingScene() const { return _pendingSceneFactory != nullptr; }

private:
    SceneManager() = default;
    ~SceneManager() = default;

    /// <summary>シーンロードの内部実装</summary>
    bool LoadSceneInternal(std::unique_ptr<SceneBase> newScene);

    /// <summary>全マネージャーをクリア</summary>
    void ClearAllManagers();
};
