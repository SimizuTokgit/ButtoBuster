#pragma once
#include "SceneBase.h"
#include "Scene.h"
#include "GameObject.h"
#include "ModeSelectScreen.h"

// モード選択のシーン タイトルの次に出す
// 表示と入力は ModeSelectScreen が持つので ここは並べるだけにしておく
class ModeSelectScene : public SceneBase {
public:
    ~ModeSelectScene() override = default;

    bool OnLoad() override {
        Scene& scene = Scene::Instance();

        auto* screenObject = scene.CreateGameObject("ModeSelectScreen");
        screenObject->AddComponent<ModeSelectScreen>();

        return true;
    }
};
