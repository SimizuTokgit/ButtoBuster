#pragma once
#include <vector>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <cmath>
#include <utility>
#include <memory>
#include <string>
#include "Object.h"
#include "Component.h"
#include "Transform.h"
#include "MonoBehaviour.h"
#include "Collision.h"

using namespace std;

// 前方宣言
class Collider;

/// <summary>
/// UnityのGameObject相当
/// コンポーネントを保持し、ゲームオブジェクトを構成する
/// 親子関係はTransformが管理する（Unityと同じ設計）
/// </summary>
class GameObject : public Object {
private:
    vector<unique_ptr<Component>> _components;
    unordered_map<type_index, Component*> _byType;

    /// <summary>子GameObjectの所有権リスト（メモリ管理用）</summary>
    vector<unique_ptr<GameObject>> _ownedChildren;

public:
    Transform* transform = nullptr;
    GameObject* gameObject = nullptr;

    /// <summary>オブジェクト名（デバッグ用）</summary>
    string name;

    /// <summary>アクティブ状態</summary>
    bool activeSelf = true;

    /// <summary>
    /// Scene::Destroy で破棄予約済みか
    /// 同じオブジェクトを二重に破棄予約しないためのフラグ
    /// </summary>
    bool pendingDestroy = false;

public:
    GameObject() {
        transform = CreateTransformComponent();
        gameObject = this;
    }

    GameObject(const string& objectName) : GameObject() {
        name = objectName;
    }

    ~GameObject() = default;

    // ===== コンポーネント管理 =====

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(is_base_of<Component, T>::value, "T must be a Component");
        auto component = make_unique<T>(forward<Args>(args)...);

        component->transform = transform;
        component->gameObject = gameObject;

        auto* rawPtr = component.get();
        _components.push_back(move(component));
        _byType[type_index(typeid(T))] = rawPtr;

        return rawPtr;
    }

    template<typename T>
    T* GetComponent() {
        auto it = _byType.find(type_index(typeid(T)));
        if (it != _byType.end()) {
            return static_cast<T*>(it->second);
        }
        return nullptr;
    }

    /// <summary>
    /// 子孫オブジェクトから指定した型のコンポーネントを検索
    /// 自分自身は検索対象に含まない
    /// 最初に見つかったコンポーネントを返す
    /// </summary>
    template<typename T>
    T* GetComponentInChildren() {
        size_t count = transform->GetChildCount();
        for (size_t i = 0; i < count; ++i) {
            auto* childTransform = transform->GetChild(i);
            auto* child = childTransform->gameObject;

            auto* component = child->GetComponent<T>();
            if (component) {
                return component;
            }

            component = child->GetComponentInChildren<T>();
            if (component) {
                return component;
            }
        }

        return nullptr;
    }

    /// <summary>
    /// 子孫オブジェクトから指定した型のコンポーネントを全て取得
    /// 自分自身は検索対象に含まない
    /// </summary>
    template<typename T>
    vector<T*> GetComponentsInChildren() {
        vector<T*> results;
        size_t count = transform->GetChildCount();
        for (size_t i = 0; i < count; ++i) {
            auto* childTransform = transform->GetChild(i);
            childTransform->gameObject->GetComponentsInChildrenInternal<T>(results);
        }
        return results;
    }

    /// <summary>
    /// 親方向に指定した型のコンポーネントを検索
    /// 自分自身から検索を開始し、見つからなければ親へ遡る
    /// UnityのGetComponentInParent相当
    /// </summary>
    template<typename T>
    T* GetComponentInParent() {
        auto* component = GetComponent<T>();
        if (component) {
            return component;
        }

        if (transform->parent) {
            return transform->parent->gameObject->GetComponentInParent<T>();
        }

        return nullptr;
    }

    /// <summary>
    /// 親方向に指定した型のコンポーネントを全て取得
    /// 自分自身から検索を開始し、ルートまで遡る
    /// UnityのGetComponentsInParent相当
    /// </summary>
    template<typename T>
    vector<T*> GetComponentsInParent() {
        vector<T*> results;
        GameObject* current = this;
        while (current) {
            auto* component = current->GetComponent<T>();
            if (component) {
                results.push_back(component);
            }
            auto* parentTransform = current->transform->parent;
            current = parentTransform ? parentTransform->gameObject : nullptr;
        }
        return results;
    }

private:
    /// <summary>
    /// GetComponentsInChildrenの内部実装（再帰用）
    /// </summary>
    template<typename T>
    void GetComponentsInChildrenInternal(vector<T*>& results) {
        auto* component = GetComponent<T>();
        if (component) {
            results.push_back(component);
        }

        size_t count = transform->GetChildCount();
        for (size_t i = 0; i < count; ++i) {
            auto* childTransform = transform->GetChild(i);
            childTransform->gameObject->GetComponentsInChildrenInternal<T>(results);
        }
    }

public:
    // ===== 子GameObject管理（Transform経由） =====

    /// <summary>
    /// 子GameObjectを作成して追加
    /// </summary>
    /// <param name="childName">子オブジェクト名</param>
    /// <returns>作成した子GameObjectへのポインタ</returns>
    GameObject* AddChild(const string& childName = "") {
        auto child = make_unique<GameObject>(childName);
        child->transform->SetParent(this->transform);

        auto* rawPtr = child.get();
        _ownedChildren.push_back(move(child));

        return rawPtr;
    }

    /// <summary>
    /// 親GameObjectを取得（Transform経由）
    /// </summary>
    GameObject* GetParent() const {
        return transform->parent ? transform->parent->gameObject : nullptr;
    }

    /// <summary>
    /// 子GameObjectの数を取得（Transform経由）
    /// </summary>
    size_t GetChildCount() const { return transform->GetChildCount(); }

    /// <summary>
    /// インデックスで子GameObjectを取得（Transform経由）
    /// </summary>
    GameObject* GetChild(size_t index) {
        auto* childTransform = transform->GetChild(index);
        return childTransform ? childTransform->gameObject : nullptr;
    }

    /// <summary>
    /// アクティブ状態を切り替える
    ///
    /// 非アクティブの間は Start / Update / DrawGizmos が呼ばれなくなる。
    /// 描画と当たり判定は各コンポーネントの enabled が別管理なので、
    /// 完全に消したい場合は Scene::Destroy を使う
    /// </summary>
    void SetActive(bool value) { activeSelf = value; }

    /// <summary>アクティブかどうか（親をたどって判定）</summary>
    bool IsActiveInHierarchy() const {
        const GameObject* current = this;
        while (current) {
            if (!current->activeSelf) return false;

            auto* parentTransform = current->transform->parent;
            current = parentTransform ? parentTransform->gameObject : nullptr;
        }
        return true;
    }

    /// <summary>
    /// 所有している子GameObjectを1つ破棄する
    /// Scene::ProcessDestroy から呼ばれる
    /// </summary>
    /// <param name="child">破棄する子</param>
    /// <returns>破棄できたらtrue</returns>
    bool RemoveOwnedChild(GameObject* child) {
        for (auto it = _ownedChildren.begin(); it != _ownedChildren.end(); ++it) {
            if (it->get() == child) {
                // unique_ptr が消えることで child のデストラクタが走り、
                // Transform のデストラクタが親子リンクを解除する
                _ownedChildren.erase(it);
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// 名前で子GameObjectを検索
    /// </summary>
    GameObject* FindChild(const string& childName) {
        size_t count = transform->GetChildCount();
        for (size_t i = 0; i < count; ++i) {
            auto* childTransform = transform->GetChild(i);
            if (childTransform->gameObject && childTransform->gameObject->name == childName) {
                return childTransform->gameObject;
            }
        }
        return nullptr;
    }

    // ===== 静的Find機能 =====

    /// <summary>
    /// 名前でGameObjectを検索（Hierarchy全体から）
    /// UnityのGameObject.Find相当
    /// </summary>
    /// <param name="objectName">検索するオブジェクト名</param>
    /// <returns>見つかったGameObject、見つからない場合はnullptr</returns>
    inline static GameObject* Find(const string& objectName);

    // ===== ライフサイクル =====

    void Start() {
        if (!activeSelf) return;

        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->Start();
            }
        }

        // 子オブジェクトのStartも呼ぶ（Transform経由）
        size_t count = transform->GetChildCount();
        for (size_t i = 0; i < count; ++i) {
            auto* childTransform = transform->GetChild(i);
            if (childTransform->gameObject) {
                childTransform->gameObject->Start();
            }
        }
    }

    void Update(float deltaTime) {
        if (!activeSelf) return;

        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->Update(deltaTime);
            }
        }

        // 子オブジェクトのUpdateも呼ぶ（Transform経由）
        size_t count = transform->GetChildCount();
        for (size_t i = 0; i < count; ++i) {
            auto* childTransform = transform->GetChild(i);
            if (childTransform->gameObject) {
                childTransform->gameObject->Update(deltaTime);
            }
        }
    }

    void DrawGizmos() {
        if (!activeSelf) return;

        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->OnDrawGizmos();
            }
        }

        size_t count = transform->GetChildCount();
        for (size_t i = 0; i < count; ++i) {
            auto* childTransform = transform->GetChild(i);
            if (childTransform->gameObject) {
                childTransform->gameObject->DrawGizmos();
            }
        }
    }

    // ===== コリジョンイベント転送（物理衝突）=====

    void OnCollisionEnter(const Collision& collision) {
        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->OnCollisionEnter(collision);
            }
        }
    }

    void OnCollisionStay(const Collision& collision) {
        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->OnCollisionStay(collision);
            }
        }
    }

    void OnCollisionExit(const Collision& collision) {
        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->OnCollisionExit(collision);
            }
        }
    }

    // ===== トリガーイベント転送（すり抜け衝突）=====

    void OnTriggerEnter(Collider* other) {
        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->OnTriggerEnter(other);
            }
        }
    }

    void OnTriggerStay(Collider* other) {
        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->OnTriggerStay(other);
            }
        }
    }

    void OnTriggerExit(Collider* other) {
        for (auto& c : _components) {
            if (auto* mb = dynamic_cast<MonoBehaviour*>(c.get())) {
                mb->OnTriggerExit(other);
            }
        }
    }

private:
    Transform* CreateTransformComponent() {
        auto component = make_unique<Transform>();

        component->transform = component.get();
        component->gameObject = this;

        auto* rawPtr = component.get();
        _components.push_back(move(component));
        _byType[type_index(typeid(Transform))] = rawPtr;

        return rawPtr;
    }
};

// ===== Component テンプレートメソッドの実装 =====
// GameObject の定義が完了した後に実装する必要がある

template<typename T>
T* Component::GetComponent() {
    return gameObject->GetComponent<T>();
}

template<typename T>
T* Component::GetComponentInChildren() {
    return gameObject->GetComponentInChildren<T>();
}

template<typename T>
std::vector<T*> Component::GetComponentsInChildren() {
    return gameObject->GetComponentsInChildren<T>();
}

template<typename T>
T* Component::GetComponentInParent() {
    return gameObject->GetComponentInParent<T>();
}

template<typename T>
std::vector<T*> Component::GetComponentsInParent() {
    return gameObject->GetComponentsInParent<T>();
}

// ===== GameObject::Find の実装 =====
// Scene の定義が必要なため、ここでインクルード
#include "Scene.h"

inline GameObject* GameObject::Find(const string& objectName) {
    return Scene::Instance().Find(objectName);
}
