#include "ArenaSceneBuilder.h"

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
#include "JustDodgeBanner.h"
#include "SlowShade.h"

namespace {
    // 光の向きと色 (r g b a) 元のステージの光に寄せて少し暖かく 暗すぎると敵の動きが読めないので明るめ
    struct LightLook {
        VECTOR direction;
        COLOR_F diffuse;
        COLOR_F ambient;
    };
    constexpr LightLook LIGHT = {
        { 0.5f, -1.0f, 0.5f },
        { 0.95f, 0.88f, 0.78f, 1.0f },
        { 0.38f, 0.36f, 0.33f, 1.0f },
    };

    // プレイヤーは闘技場の真ん中の地面から、これだけ上に出す
    constexpr float PLAYER_START_LIFT = 50.0f;
}

ArenaSceneBuilder::Arena ArenaSceneBuilder::Build(int needleCount) {
    Scene& scene = Scene::Instance();
    Arena arena;

    // 見える近さと遠さは Camera の初めの値のまま
    auto* cameraObject = scene.CreateGameObject("MainCamera");
    auto* camera = cameraObject->AddComponent<Camera>();
    camera->SetAsMainCamera();
    camera->SetupDirectionalLight(LIGHT.direction, LIGHT.diffuse, LIGHT.ambient);

    arena.cameraFollow = cameraObject->AddComponent<CameraFollow>();

    // 3Dサウンドの聞き手はカメラに付ける
    auto* listener = cameraObject->AddComponent<AudioListener>();
    listener->Register();

    if (!StageBuilder::Build(camera)) return arena;

    auto* effectObject = scene.CreateGameObject("EffectManager");
    auto* effects = effectObject->AddComponent<EffectManager>();
    effects->Initialize(arena.cameraFollow);

    // 壁が割れたときの演出と音 壁割りの知らせを受けて出すので、変えるときはこの 2 つを書き換える
    auto* wallBreakObject = scene.CreateGameObject("WallBreakEffects");
    ArenaWall::GetBreakEvents().AddObserver(wallBreakObject->AddComponent<WallBreakEffectObserver>());
    ArenaWall::GetBreakEvents().AddObserver(wallBreakObject->AddComponent<WallBreakSoundObserver>());

    auto* needleObject = scene.CreateGameObject("NeedlePool");
    auto* needles = needleObject->AddComponent<NeedlePool>();
    needles->Initialize(needleCount);

    VECTOR start = StageBuilder::GetArenaCenter();
    float groundY = 0.0f;
    if (StageBuilder::FindGroundHeight(start.x, start.z, groundY)) start.y = groundY + PLAYER_START_LIFT;

    arena.player = PlayerFactory::Create(start);
    if (!arena.player) return arena;
    arena.controller = arena.player->GetComponent<PlayerController>();

    // 移動はカメラから見た向き 視点を回す操作もコントローラーから渡す
    arena.cameraFollow->target = arena.player->transform;
    if (arena.controller) arena.controller->SetCamera(arena.cameraFollow);

    if (auto* boundary = scene.FindFirstObjectByType<ArenaBoundary>()) {
        boundary->SetViewer(arena.player->transform);
    }

    return arena;
}

void ArenaSceneBuilder::AddJustDodgeUi(GameObject* uiObject, Player* player) {
    // ジャスト回避の文字は、プレイヤーの知らせを受けて出す
    auto* justDodgeBanner = uiObject->AddChild("JustDodgeBanner")->AddComponent<JustDodgeBanner>();
    justDodgeBanner->Setup();
    player->GetJustDodgeEvents().AddObserver(justDodgeBanner);

    // ジャスト回避のあとの反撃で敵がゆっくりになっている間、画面を暗くする
    auto* slowShade = uiObject->AddChild("SlowShade")->AddComponent<SlowShade>();
    slowShade->Setup(player);
}
