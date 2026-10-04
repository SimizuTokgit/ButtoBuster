#pragma once
#include "SceneBase.h"
#include "Scene.h"
#include "GameObject.h"
#include "TitleScreen.h"

// タイトルシーン
// 表示と入力は TitleScreen が持つので ここは並べるだけにしておく
class TitleScene : public SceneBase {
public:
    ~TitleScene() override = default;

    bool OnLoad() override {
        Scene& scene = Scene::Instance();

        auto* titleObject = scene.CreateGameObject("TitleScreen");
        titleObject->AddComponent<TitleScreen>();

        return true;
    }
};
