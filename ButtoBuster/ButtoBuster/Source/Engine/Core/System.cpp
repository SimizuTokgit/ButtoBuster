#include "System.h"

#include "DxLib.h"

#include "Time.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Skybox.h"
#include "PhysicsManager.h"
#include "RenderManager.h"
#include "AudioManager.h"
#include "SoundManager.h"
#include "ParticleManager.h"
#include "UIManager.h"
#include "InputSystem.h"
#include "DebugMenu.h"

bool System::Main(const std::function<bool()>& loadFirstScene) {
    if (!Initialize(loadFirstScene)) return false;

    MainLoop();

    Terminate();

    return true;
}

bool System::Initialize(const std::function<bool()>& loadFirstScene) {
    ChangeWindowMode(TRUE);
    SetGraphMode(GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT, 32);
    SetMainWindowText("ButtoBuster");

    // ウィンドウが非アクティブでもゲームを動かす
    SetAlwaysRunFlag(TRUE);

    if (DxLib_Init() == -1) return false;

    SetDrawScreen(DX_SCREEN_BACK);

    // サウンドを一括ロード
    SoundManager::Instance().Init();

    // 制作用のチートはここか各シーンで足す
    DebugMenu::Instance().AddToggle(KEY_INPUT_F2, "当たり判定");

    if (!loadFirstScene || !loadFirstScene()) return false;

    _prevTime = GetNowCount();

    return true;
}

void System::MainLoop() {
    Scene& scene = Scene::Instance();

    while (ProcessMessage() == 0 && !_quitRequested) {
        // 実時間の deltaTime を測る
        int nowTime = GetNowCount();
        float unscaledDeltaTime = (nowTime - _prevTime) / 1000.0f;
        _prevTime = nowTime;

        if (unscaledDeltaTime > MAX_DELTA_TIME) {
            unscaledDeltaTime = MAX_DELTA_TIME;
        }

        // ヒットストップやスロー再生の倍率はここで掛かる
        Time::Update(unscaledDeltaTime);
        float deltaTime = Time::DeltaTime();

        // 入力はフレームの先頭で1回だけ読む
        InputSystem::Instance().Update();
        DebugMenu::Instance().Update();

        if (InputSystem::Instance().KeyPressed(KEY_INPUT_ESCAPE)) {
            _quitRequested = true;
        }

        ClearDrawScreen();

        // 物理更新 (重力 コリジョン)
        PhysicsManager::Instance().Update(deltaTime);

        // 全GameObjectのUpdate
        scene.Update(deltaTime);

        // パーティクル更新
        ParticleManager::Instance().UpdateAll(deltaTime);

        // カメラ更新
        auto* camera = Camera::GetMain();
        if (camera) {
            camera->Apply();

            // 空を最初に描く
            auto* skybox = camera->GetSkybox();
            if (skybox && skybox->IsLoaded()) {
                skybox->SetCameraPosition(camera->GetPosition());
                skybox->Render(camera->nearClipPlane, camera->farClipPlane);
            }
        }

        // オーディオ更新 (3D位置の反映)
        AudioManager::Instance().Update();

        // BGMのフェードは止まっている間も進めたいので実時間で回す
        SoundManager::Instance().Update(Time::UnscaledDeltaTime());

        // 3D描画 -> パーティクル -> UI の順
        RenderManager::Instance().RenderAll();
        ParticleManager::Instance().RenderAll();
        UIManager::Instance().RenderAll();

        if (DebugMenu::Instance().GetToggle("当たり判定")) {
            scene.DrawGizmos();
        }
        DebugMenu::Instance().Render();

        ScreenFlip();

        // 破棄予約をまとめて処理 フレーム末尾で安全に消す
        scene.ProcessDestroy();

        // シーン遷移もフレーム末尾で行う
        SceneManager::Instance().ProcessSceneTransition();
    }
}

void System::Terminate() {
    // マネージャーが生きているうちに GameObject を片付ける
    Scene::Instance().Clear();

    // DxLib_End の前にサウンドハンドルを解放する
    SoundManager::Instance().Terminate();

    DxLib_End();
}
