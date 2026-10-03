#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

class Transform;

// 戦える範囲の端に見えない壁があることを知らせる光の幕
// 近づいた部分だけ浮かび上がらせる 常に見えていると景色の邪魔になる
class ArenaBoundary : public Renderer {
private:
    static constexpr int SEGMENT_COUNT = 72;

    // これより離れていれば見えない
    static constexpr float VISIBLE_DISTANCE = 700.0f;

    static constexpr float WALL_BELOW = 200.0f;
    static constexpr float WALL_ABOVE = 450.0f;

    VECTOR _center = VGet(0.0f, 0.0f, 0.0f);
    float _radius = 0.0f;
    Transform* _viewer = nullptr;
    std::vector<VERTEX3D> _vertices;

public:
    void Setup(VECTOR center, float radius);

    // 誰に近いところを光らせるか ふつうはプレイヤー
    void SetViewer(Transform* viewer) { _viewer = viewer; }

    void Render() override;
};
