#include "TutorialScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "ArenaSceneBuilder.h"
#include "PhaseDirector.h"
#include "TutorialDirector.h"
#include "TutorialScreen.h"
#include "Hud.h"

namespace {
    // 練習の Bee は撃ってこないが、針の置き場は本番と同じく置いておく
    constexpr int NEEDLE_COUNT = 4;
}

bool TutorialScene::OnLoad() {
    Scene& scene = Scene::Instance();

    // カメラ 光 ステージ 演出 プレイヤーは、本番と同じ組み立て (ArenaSceneBuilder)
    ArenaSceneBuilder::Arena arena = ArenaSceneBuilder::Build(NEEDLE_COUNT);
    if (!arena.player) return false;

    // 練習の相手を出すのと片付けるのは、フェーズを進めない練習の段の PhaseDirector に任せる
    // 敵の AI は本番と同じく、ここから攻撃の番をもらう
    auto* directorObject = scene.CreateGameObject("PhaseDirector");
    auto* phases = directorObject->AddComponent<PhaseDirector>();
    phases->InitializePractice(arena.player, arena.controller);

    auto* tutorialObject = scene.CreateGameObject("TutorialDirector");
    auto* tutorial = tutorialObject->AddComponent<TutorialDirector>();
    tutorial->Initialize(arena.player, arena.controller, arena.cameraFollow, phases);

    // 画面の表示 フェーズの表示は練習の段では出ない
    auto* uiObject = scene.CreateGameObject("UI");
    auto* hud = uiObject->AddComponent<Hud>();
    hud->Setup(arena.player, phases);

    auto* tutorialScreen = uiObject->AddChild("TutorialScreen")->AddComponent<TutorialScreen>();
    tutorialScreen->Setup(tutorial);

    ArenaSceneBuilder::AddJustDodgeUi(uiObject, arena.player);

    return true;
}
