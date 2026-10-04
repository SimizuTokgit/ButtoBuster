#pragma once
#include <vector>
#include <string>
#include <memory>
#include <type_traits>

class GameObject;
class Component;

/// <summary>
/// UnityのScene/Hierarchy相当
/// 全てのルートGameObjectを管理し、Find機能を提供する
/// シングルトンパターンで実装
/// </summary>
class Scene {
private:
    /// <summary>ルートGameObjectのリスト</summary>
    std::vector<std::unique_ptr<GameObject>> _rootObjects;

    /// <summary>破棄予約されたGameObject（フレーム末にまとめて処理する）</summary>
    std::vector<GameObject*> _destroyQueue;

    /// <summary>
    /// まだ Start が呼ばれていないGameObject
    ///
    /// ゲーム中に生成したオブジェクトは、シーン開始時の Scene::Start を
    /// 通っていないため Start が呼ばれない。
    /// 生成した次のフレームの頭でまとめて呼ぶ（Unityと同じ挙動）
    /// </summary>
    std::vector<GameObject*> _pendingStart;

public:
    /// <summary>
    /// シングルトンインスタンスを取得
    /// </summary>
    static Scene& Instance() {
        static Scene instance;
        return instance;
    }

    // コピー禁止
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // ===== GameObject管理 =====

    /// <summary>
    /// 新しいGameObjectを作成してシーンに追加
    /// </summary>
    /// <param name="name">オブジェクト名</param>
    /// <returns>作成したGameObjectへのポインタ</returns>
    GameObject* CreateGameObject(const std::string& name = "");

    /// <summary>
    /// 全てのルートGameObjectを取得
    /// </summary>
    const std::vector<std::unique_ptr<GameObject>>& GetRootObjects() const;

    /// <summary>
    /// GameObjectの破棄を予約する（UnityのDestroy相当）
    ///
    /// Update中に即座に破棄すると、走査中のリストが壊れたり
    /// 破棄したオブジェクト自身の処理が続いてクラッシュする。
    /// そのため予約だけ行い、実際の破棄はフレーム末のProcessDestroyで行う
    /// </summary>
    /// <param name="obj">破棄するGameObject（子オブジェクトも一緒に消える）</param>
    void Destroy(GameObject* obj);

    /// <summary>
    /// 予約された破棄をまとめて実行する（フレーム末に呼ぶ）
    /// </summary>
    void ProcessDestroy();

    /// <summary>
    /// ゲーム中に生成されたGameObjectの Start をまとめて呼ぶ
    /// Scene::Update の先頭から自動的に呼ばれる
    /// </summary>
    void ProcessPendingStart();

    // ===== Find機能 =====

    /// <summary>
    /// 名前でGameObjectを検索（Hierarchy全体から）
    /// </summary>
    /// <param name="name">検索するオブジェクト名</param>
    /// <returns>見つかったGameObject、見つからない場合はnullptr</returns>
    GameObject* Find(const std::string& name);

    /// <summary>
    /// 指定した型のコンポーネントを持つ最初のGameObjectからそのコンポーネントを取得
    /// </summary>
    template<typename T>
    T* FindFirstObjectByType();

    /// <summary>
    /// 指定した型のコンポーネントを全て取得
    /// </summary>
    template<typename T>
    std::vector<T*> FindObjectsByType();

    // ===== ライフサイクル =====

    /// <summary>
    /// 全てのGameObjectのStartを呼び出す
    /// </summary>
    void Start();

    /// <summary>
    /// 全てのGameObjectのUpdateを呼び出す
    /// </summary>
    void Update(float deltaTime);

    /// <summary>
    /// 全てのGameObjectのOnDrawGizmosを呼び出す
    /// </summary>
    void DrawGizmos();

    /// <summary>
    /// 全てのルートGameObjectを破棄（シーン切り替え時に使用）
    /// </summary>
    void Clear() {
        _destroyQueue.clear();
        _pendingStart.clear();
        _rootObjects.clear();
    }

private:
    Scene() = default;
    ~Scene() = default;

    /// <summary>
    /// 再帰的に名前でGameObjectを検索
    /// </summary>
    GameObject* FindRecursive(GameObject* obj, const std::string& name);

    /// <summary>
    /// 再帰的にコンポーネントを検索
    /// </summary>
    template<typename T>
    T* FindFirstObjectByTypeRecursive(GameObject* obj);

    /// <summary>
    /// 再帰的に全てのコンポーネントを収集
    /// </summary>
    template<typename T>
    void FindObjectsByTypeRecursive(GameObject* obj, std::vector<T*>& results);
};

// テンプレートメソッドの実装（ヘッダーに配置が必要）
#include "GameObject.h"

template<typename T>
T* Scene::FindFirstObjectByType() {
    for (auto& obj : _rootObjects) {
        // ルートオブジェクト自身をチェック
        auto* component = obj->GetComponent<T>();
        if (component) {
            return component;
        }

        // 子孫を再帰的に検索
        auto* found = FindFirstObjectByTypeRecursive<T>(obj.get());
        if (found) {
            return found;
        }
    }
    return nullptr;
}

template<typename T>
T* Scene::FindFirstObjectByTypeRecursive(GameObject* obj) {
    size_t childCount = obj->GetChildCount();
    for (size_t i = 0; i < childCount; ++i) {
        auto* child = obj->GetChild(i);

        // 子オブジェクトをチェック
        auto* component = child->GetComponent<T>();
        if (component) {
            return component;
        }

        // 孫以降を再帰的に検索
        auto* found = FindFirstObjectByTypeRecursive<T>(child);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

template<typename T>
std::vector<T*> Scene::FindObjectsByType() {
    std::vector<T*> results;
    for (auto& obj : _rootObjects) {
        // ルートオブジェクト自身をチェック
        auto* component = obj->GetComponent<T>();
        if (component) {
            results.push_back(component);
        }

        // 子孫を再帰的に収集
        FindObjectsByTypeRecursive<T>(obj.get(), results);
    }
    return results;
}

template<typename T>
void Scene::FindObjectsByTypeRecursive(GameObject* obj, std::vector<T*>& results) {
    size_t childCount = obj->GetChildCount();
    for (size_t i = 0; i < childCount; ++i) {
        auto* child = obj->GetChild(i);

        // 子オブジェクトをチェック
        auto* component = child->GetComponent<T>();
        if (component) {
            results.push_back(component);
        }

        // 孫以降を再帰的に収集
        FindObjectsByTypeRecursive<T>(child, results);
    }
}

// ===== Object テンプレートメソッドの実装 =====
// Scene の定義が完了した後に実装する必要がある

template<typename T>
T* Object::FindFirstObjectByType() {
    return Scene::Instance().FindFirstObjectByType<T>();
}

template<typename T>
std::vector<T*> Object::FindObjectsByType() {
    return Scene::Instance().FindObjectsByType<T>();
}
