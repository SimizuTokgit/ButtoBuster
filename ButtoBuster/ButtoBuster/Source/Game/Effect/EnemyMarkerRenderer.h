#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

// 敵の足元の印 地面の色に敵が溶けて見えにくいので、毎フレーム全員の足元に描く
// 黒い影 (真ん中が濃く外へ薄くなる丸) と、その縁の色の付いた輪
// 飛んでいる敵や吹き飛んで宙にいる敵は、真下の地面に描く 倒された敵には描かない
// 色と大きさの数値は EnemyMarkerRenderer.cpp の先頭
class EnemyMarkerRenderer : public Renderer {
private:
    std::vector<VERTEX3D> _vertices;

public:
    void Setup();
    void Render() override;

private:
    void BuildMarker(VECTOR ground, float bodyRadius);
};
