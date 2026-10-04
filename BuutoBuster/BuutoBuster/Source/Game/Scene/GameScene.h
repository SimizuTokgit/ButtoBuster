#pragma once
#include "SceneBase.h"

// ゲームシーン
// プレイヤー 敵 地形などゲーム本編の GameObject を組み立てる
class GameScene : public SceneBase {
public:
    ~GameScene() override = default;

    bool OnLoad() override;
};
