#include "Scene.h"
#include "GameObject.h"

GameObject* Scene::CreateGameObject(const std::string& name) {
    auto obj = std::make_unique<GameObject>(name);
    auto* rawPtr = obj.get();
    _rootObjects.push_back(std::move(obj));

    // ゲーム中に生成された場合に備えて Start 待ちへ積む。
    // シーン開始時の一括 Start を通る分は Scene::Start 側で破棄される
    _pendingStart.push_back(rawPtr);

    return rawPtr;
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetRootObjects() const {
    return _rootObjects;
}

void Scene::Destroy(GameObject* obj) {
    if (!obj) return;

    // 二重予約を防ぐ
    if (obj->pendingDestroy) return;
    obj->pendingDestroy = true;

    // 予約した時点で更新も描画も止めておく
    // （実際に消えるのはフレーム末なので、それまで動き続けないようにする）
    obj->activeSelf = false;

    _destroyQueue.push_back(obj);
}

void Scene::ProcessDestroy() {
    if (_destroyQueue.empty()) return;

    // 破棄処理の途中で新たな破棄が予約される場合に備えて、
    // 一旦手元へ移してからキューを空にする
    std::vector<GameObject*> targets;
    targets.swap(_destroyQueue);

    for (auto* obj : targets) {
        if (!obj) continue;

        // ルートオブジェクトなら、シーンの所有リストから外す
        bool removed = false;
        for (auto it = _rootObjects.begin(); it != _rootObjects.end(); ++it) {
            if (it->get() == obj) {
                _rootObjects.erase(it);
                removed = true;
                break;
            }
        }
        if (removed) continue;

        // 子オブジェクトなら、親の所有リストから外す
        auto* parentObject = obj->GetParent();
        if (parentObject) {
            parentObject->RemoveOwnedChild(obj);
        }
    }
}

GameObject* Scene::Find(const std::string& name) {
    for (auto& obj : _rootObjects) {
        // ルートオブジェクト自身をチェック
        if (obj->name == name) {
            return obj.get();
        }

        // 子孫を再帰的に検索
        auto* found = FindRecursive(obj.get(), name);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

GameObject* Scene::FindRecursive(GameObject* obj, const std::string& name) {
    // 子オブジェクトを検索
    size_t childCount = obj->GetChildCount();
    for (size_t i = 0; i < childCount; ++i) {
        auto* child = obj->GetChild(i);
        if (child->name == name) {
            return child;
        }

        // 孫以降を再帰的に検索
        auto* found = FindRecursive(child, name);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

void Scene::Start() {
    // CreateGameObject は生成したルートを必ず _pendingStart へ積むため、
    // シーン構築直後は「まだ Start していない全ルート」がそこに揃っている。
    // よって初期化も遅延Startと同じ処理で済む
    ProcessPendingStart();
}

void Scene::Update(float deltaTime) {
    // ゲーム中に生成されたオブジェクトの Start を先に済ませる
    ProcessPendingStart();

    // Update の中で敵が生成されると _rootObjects が再確保され、
    // 走査中の参照が無効になってクラッシュする。
    // 更新対象を先に控えてから回す
    // （破棄はフレーム末までは実行されないので、控えたポインタは今フレーム中は有効）
    std::vector<GameObject*> targets;
    targets.reserve(_rootObjects.size());
    for (auto& obj : _rootObjects) {
        targets.push_back(obj.get());
    }

    for (auto* obj : targets) {
        if (!obj) continue;
        obj->Update(deltaTime);
    }
}

void Scene::ProcessPendingStart() {
    if (_pendingStart.empty()) return;

    // Start の中でさらにオブジェクトが生成される場合に備えて、
    // 一旦手元へ移してからリストを空にする
    std::vector<GameObject*> targets;
    targets.swap(_pendingStart);

    for (auto* obj : targets) {
        if (!obj) continue;
        if (obj->pendingDestroy) continue;

        obj->Start();
    }
}

void Scene::DrawGizmos() {
    // Update と同じ理由で、走査対象を先に控えてから回す
    std::vector<GameObject*> targets;
    targets.reserve(_rootObjects.size());
    for (auto& obj : _rootObjects) {
        targets.push_back(obj.get());
    }

    for (auto* obj : targets) {
        if (!obj) continue;
        obj->DrawGizmos();
    }
}
