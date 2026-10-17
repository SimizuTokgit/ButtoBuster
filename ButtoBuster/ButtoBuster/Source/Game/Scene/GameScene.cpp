#include "GameScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "ArenaSceneBuilder.h"
#include "Player.h"
#include "PhaseDirector.h"
#include "VictoryEffectObserver.h"
#include "Hud.h"
#include "PhaseBanner.h"
#include "ResultScreen.h"
#include "ChainCounter.h"
#include "DebugCheats.h"
#include "ChargePoseDebugger.h"

namespace {
    // Bee は最大 3 体 1 体が続けて撃っても足りる数
    constexpr int NEEDLE_COUNT = 16;
}

bool GameScene::OnLoad() {
    Scene& scene = Scene::Instance();

    // カメラ 光 ステージ 演出 プレイヤーは、チュートリアルと同じ組み立て (ArenaSceneBuilder)
    ArenaSceneBuilder::Arena arena = ArenaSceneBuilder::Build(NEEDLE_COUNT);
    if (!arena.player) return false;

    auto* directorObject = scene.CreateGameObject("PhaseDirector");
    auto* director = directorObject->AddComponent<PhaseDirector>();
    director->Initialize(arena.player, arena.controller);

    // 勝ったときの演出 勝ちの知らせを受けて出すので、変えるときはこれを書き換える
    auto* victoryObject = scene.CreateGameObject("VictoryEffects");
    director->GetVictoryEvents().AddObserver(victoryObject->AddComponent<VictoryEffectObserver>());

    auto* uiObject = scene.CreateGameObject("UI");
    auto* hud = uiObject->AddComponent<Hud>();
    hud->Setup(arena.player, director);

    // 1つの GameObject は同じ型を1つしか持てないので、表示ごとに子に分ける
    auto* banner = uiObject->AddChild("PhaseBanner")->AddComponent<PhaseBanner>();
    banner->Setup(director);

    auto* result = uiObject->AddChild("ResultScreen")->AddComponent<ResultScreen>();
    result->Setup(arena.player, director);

    // 連鎖の数は、プレイヤーの連鎖の知らせを受けて出す
    auto* chainCounter = uiObject->AddChild("ChainCounter")->AddComponent<ChainCounter>();
    chainCounter->Setup();
    arena.player->GetChainEvents().AddObserver(chainCounter);

    // ジャスト回避の文字と、反撃の間の暗い幕 チュートリアルと同じ
    ArenaSceneBuilder::AddJustDodgeUi(uiObject, arena.player);

    auto* debugObject = scene.CreateGameObject("DebugCheats");
    auto* cheats = debugObject->AddComponent<DebugCheats>();
    cheats->Setup(arena.player, director, hud, arena.cameraFollow);

    auto* poseDebugger = debugObject->AddComponent<ChargePoseDebugger>();
    poseDebugger->Setup(arena.player);

    return true;
}
