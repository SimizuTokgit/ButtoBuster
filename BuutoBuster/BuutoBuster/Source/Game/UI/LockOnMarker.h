#pragma once
#include "UIImage.h"

class PlayerController;
class Character;

// ロックオンしている相手に重ねる印と、画面の上に出す相手の名前
// 印は相手の周りを 4 つの三角で囲んでゆっくり回す 付けた瞬間は外から縮んで収まる
class LockOnMarker : public UIImage {
private:
    const PlayerController* _controller = nullptr;

    // 前のフレームに印を出した相手 変わったら縮む動きをやり直す
    // 比べるだけで中身は読まない もう消えているかもしれないので
    const Character* _shownTarget = nullptr;

    float _spinDegree = 0.0f;
    float _appear = 0.0f;

public:
    void Setup(const PlayerController* controller);

    void Update(float deltaTime) override;
    void Render() override;

private:
    void DrawReticle(float x, float y) const;
    void DrawTargetName(const Character& target, int screenWidth) const;
};
