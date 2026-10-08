#pragma once
#include "UIImage.h"

class TutorialDirector;

// チュートリアルの指示を画面の上に出す 右上には長押しでスキップの案内
// 何を出すかは TutorialDirector から読むだけで、進め方は知らない
// 場所と色は .cpp の先頭に並べてある
class TutorialScreen : public UIImage {
private:
    TutorialDirector* _director = nullptr;

public:
    void Setup(TutorialDirector* director);
    void Render() override;

private:
    void DrawPanel(int screenWidth);
    void DrawSkip(int screenWidth);
};
