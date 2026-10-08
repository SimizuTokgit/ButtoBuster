#include "TutorialScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "Camera.h"
#include "CameraFollow.h"
#include "AudioListener.h"
#include "StageBuilder.h"
#include "ArenaBoundary.h"
#include "ArenaWall.h"
#include "WallBreakEffectObserver.h"
#include "WallBreakSoundObserver.h"
#include "EffectManager.h"
#include "NeedlePool.h"
#include "PlayerFactory.h"
#include "Player.h"
#include "PlayerController.h"
#include "PhaseDirector.h"
#include "TutorialDirector.h"
#include "TutorialScreen.h"
#include "Hud.h"
#include "JustDodgeBanner.h"
#include "SlowShade.h"

namespace {
    // 練習の Bee は撃ってこないが、針の置き場は本番と同じく置いておく
    constexpr int NEEDLE_COUNT = 4;
}

bool TutorialScene::OnLoad() {
    Scene& scene = Scene::Instance();

    // カメラ 光 ステージ 演出 プレイヤーは本番 (GameScene.cpp) と同じ組み立て 変えるときは両方そろえる
    auto* cameraObject = scene.CreateGameObject("MainCamera");
    auto* camera = cameraObject->AddComponent<Camera>();
    camera->SetAsMainCamera();
    camera->nearClipPlane = 10.0f;
    camera->farClipPlane = 50000.0f;
    camera->SetupDirectionalLight(
        VGet(0.5f, -1.0f, 0.5f),
        GetColorF(0.95f, 0.88f, 0.78f, 1.0f),
        GetColorF(0.38f, 0.36f, 0.33f, 1.0f));

    auto* cameraFollow = cameraObject->AddComponent<CameraFollow>();

    auto* listener = cameraObject->AddComponent<AudioListener>();
    listener->Register();

    if (!StageBuilder::Build(camera)) return false;

    auto* effectObject = scene.CreateGameObject("EffectManager");
    auto* effects = effectObject->AddComponent<EffectManager>();
    effects->Initialize(cameraFollow);

    auto* wallBreakObject = scene.CreateGameObject("WallBreakEffects");
    ArenaWall::GetBreakEvents().AddObserver(wallBreakObject->AddComponent<WallBreakEffectObserver>());
    ArenaWall::GetBreakEvents().AddObserver(wallBreakObject->AddComponent<WallBreakSoundObserver>());

    auto* needleObject = scene.CreateGameObject("NeedlePool");
    auto* needles = needleObject->AddComponent<NeedlePool>();
    needles->Initialize(NEEDLE_COUNT);

    VECTOR start = StageBuilder::GetArenaCenter();
    float groundY = 0.0f;
    if (StageBuilder::FindGroundHeight(start.x, start.z, groundY)) start.y = groundY + 50.0f;

    auto* player = PlayerFactory::Create(start);
    if (!player) return false;
    auto* controller = player->GetComponent<PlayerController>();

    cameraFollow->target = player->transform;
    if (controller) controller->SetCamera(cameraFollow);

    if (auto* boundary = scene.FindFirstObjectByType<ArenaBoundary>()) {
        boundary->SetViewer(player->transform);
    }

    // 練習の相手を出すのと片付けるのは、フェーズを進めない練習の段の PhaseDirector に任せる
    // 敵の AI は本番と同じく、ここから攻撃の番をもらう
    auto* directorObject = scene.CreateGameObject("PhaseDirector");
    auto* phases = directorObject->AddComponent<PhaseDirector>();
    phases->InitializePractice(player, controller);

    auto* tutorialObject = scene.CreateGameObject("TutorialDirector");
    auto* tutorial = tutorialObject->AddComponent<TutorialDirector>();
    tutorial->Initialize(player, controller, cameraFollow, phases);

    // 画面の表示 フェーズの表示は練習の段では出ない
    auto* uiObject = scene.CreateGameObject("UI");
    auto* hud = uiObject->AddComponent<Hud>();
    hud->Setup(player, phases);

    auto* tutorialScreen = uiObject->AddChild("TutorialScreen")->AddComponent<TutorialScreen>();
    tutorialScreen->Setup(tutorial);

    auto* justDodgeBanner = uiObject->AddChild("JustDodgeBanner")->AddComponent<JustDodgeBanner>();
    justDodgeBanner->Setup();
    player->GetJustDodgeEvents().AddObserver(justDodgeBanner);

    auto* slowShade = uiObject->AddChild("SlowShade")->AddComponent<SlowShade>();
    slowShade->Setup(player);

    return true;
}
