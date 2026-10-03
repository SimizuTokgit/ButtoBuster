#include "SceneManager.h"
#include "SceneBase.h"
#include "Scene.h"
#include "PhysicsManager.h"
#include "RenderManager.h"
#include "AudioManager.h"
#include "ParticleManager.h"
#include "UIManager.h"
#include "DebugMenu.h"

using namespace std;

bool SceneManager::LoadSceneInternal(unique_ptr<SceneBase> newScene) {
    // 現在のシーンをアンロード
    if (_currentScene) {
        _currentScene->OnUnload();
    }

    // 全マネージャーをクリア
    ClearAllManagers();

    // 新しいシーンをロード
    _currentScene = move(newScene);
    if (!_currentScene->OnLoad()) {
        return false;
    }

    // 全GameObjectのStartを実行
    Scene::Instance().Start();

    return true;
}

void SceneManager::ProcessSceneTransition() {
    if (_pendingSceneFactory) {
        auto factory = move(_pendingSceneFactory);
        _pendingSceneFactory = nullptr;
        LoadSceneInternal(factory());
    }
}

void SceneManager::ClearAllManagers() {
    Scene::Instance().Clear();
    PhysicsManager::Instance().Clear();
    RenderManager::Instance().Clear();
    AudioManager::Instance().Clear();
    ParticleManager::Instance().Clear();
    UIManager::Instance().Clear();
    DebugMenu::Instance().Clear();
}
