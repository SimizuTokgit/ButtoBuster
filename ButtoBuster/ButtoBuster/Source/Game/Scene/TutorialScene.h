#pragma once
#include "SceneBase.h"

// チュートリアル タイトルと本番 (GameScene) の間に毎回入る
// 本番と同じ闘技場で、画面の上に出る指示をやって覚える 進め方は TutorialDirector、段の中身は TutorialSteps.cpp
// Enter / Start の長押しで、いつでも本番へ飛ばせる
class TutorialScene : public SceneBase {
public:
    ~TutorialScene() override = default;

    bool OnLoad() override;
};
