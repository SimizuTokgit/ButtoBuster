#include "GameScene.h"

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
#include "Hud.h"
#include "PhaseBanner.h"
#include "ResultScreen.h"
#include "ChainCounter.h"
#include "JustDodgeBanner.h"
#include "SlowShade.h"
#include "DebugCheats.h"
#include "ChargePoseDebugger.h"

namespace {
    // Bee は最大 3 体 1 体が続けて撃っても足りる数
    constexpr int NEEDLE_COUNT = 16;
}

bool GameScene::OnLoad() {
    Scene& scene = Scene::Instance();

    auto* cameraObject = scene.CreateGameObject("MainCamera");
    auto* camera = cameraObject->AddComponent<Camera>();
    camera->SetAsMainCamera();
    camera->nearClipPlane = 10.0f;
    camera->farClipPlane = 50000.0f;
    // 元のステージの光に寄せて少し暖かく 暗すぎると敵の動きが読めないので明るめ
    camera->SetupDirectionalLight(
        VGet(0.5f, -1.0f, 0.5f),
        GetColorF(0.95f, 0.88f, 0.78f, 1.0f),
        GetColorF(0.38f, 0.36f, 0.33f, 1.0f));

    auto* cameraFollow = cameraObject->AddComponent<CameraFollow>();

    // 3Dサウンドの聞き手はカメラに付ける
    auto* listener = cameraObject->AddComponent<AudioListener>();
    listener->Register();

    if (!StageBuilder::Build(camera)) return false;

    auto* effectObject = scene.CreateGameObject("EffectManager");
    auto* effects = effectObject->AddComponent<EffectManager>();
    effects->Initialize(cameraFollow);

    // 壁が割れたときの演出と音 壁割りの知らせを受けて出すので、変えるときはこの 2 つを書き換える
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

    // 移動はカメラから見た向き 視点を回す操作もコントローラーから渡す
    cameraFollow->target = player->transform;
    if (controller) controller->SetCamera(cameraFollow);

    if (auto* boundary = scene.FindFirstObjectByType<ArenaBoundary>()) {
        boundary->SetViewer(player->transform);
    }

    auto* directorObject = scene.CreateGameObject("PhaseDirector");
    auto* director = directorObject->AddComponent<PhaseDirector>();
    director->Initialize(player, controller);

    auto* uiObject = scene.CreateGameObject("UI");
    auto* hud = uiObject->AddComponent<Hud>();
    hud->Setup(player, director);

    // 1つの GameObject は同じ型を1つしか持てないので、表示ごとに子に分ける
    auto* banner = uiObject->AddChild("PhaseBanner")->AddComponent<PhaseBanner>();
    banner->Setup(director);

    auto* result =uiObject->AddChild("ResultScreen")->AddComponent<ResultScreen>();
    result->Setup(player, director);

    // 連鎖の数は、プレイヤーの連鎖の知らせを受けて出す
    auto* chainCounter = uiObject->AddChild("ChainCounter")->AddComponent<ChainCounter>();
    chainCounter->Setup();
    player->GetChainEvents().AddObserver(chainCounter);

    // ジャスト回避の文字も、プレイヤーの知らせを受けて出す
    auto* justDodgeBanner = uiObject->AddChild("JustDodgeBanner")->AddComponent<JustDodgeBanner>();
    justDodgeBanner->Setup();
    player->GetJustDodgeEvents().AddObserver(justDodgeBanner);

    // ジャスト回避のあとの反撃で敵がゆっくりになっている間、画面を暗くする
    auto* slowShade = uiObject->AddChild("SlowShade")->AddComponent<SlowShade>();
    slowShade->Setup(player);

    auto* debugObject = scene.CreateGameObject("DebugCheats");
    auto* cheats = debugObject->AddComponent<DebugCheats>();
    cheats->Setup(player, director, hud);

    auto* poseDebugger = debugObject->AddComponent<ChargePoseDebugger>();
    poseDebugger->Setup(player);

    return true;
}
