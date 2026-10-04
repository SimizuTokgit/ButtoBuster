#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

class Transform;

// 戦える範囲の端に壁があることを知らせる光の幕
// 近づいた部分だけ浮かび上がらせる 常に見えていると景色の邪魔になる
// 吹き飛んだ体がぶつかった所は、離れていても一瞬光らせる 何もない所で跳ね返ったように見えないように
//
// シーンに 1 つだけ置き、どこからでも Get() で呼べるようにする 壁そのものの決まりは ArenaWall
class ArenaBoundary : public Renderer {
private:
    static inline ArenaBoundary* _instance = nullptr;

    static constexpr int SEGMENT_COUNT = 72;

    // これより離れていれば見えない
    static constexpr float VISIBLE_DISTANCE = 700.0f;

    static constexpr float WALL_BELOW = 200.0f;
    static constexpr float WALL_ABOVE = 450.0f;

    // ぶつかった所を光らせる 同時に覚えておく数と、消えるまでの秒数と、左右に広げる角度 (度)
    static constexpr int IMPACT_COUNT = 8;
    static constexpr float IMPACT_TIME = 0.5f;
    static constexpr float IMPACT_SPREAD = 8.0f;

    struct Impact {
        // 中心から見た向き ラジアン
        float angle = 0.0f;
        float height = 0.0f;

        // 0 なら使っていない
        float strength = 0.0f;

        // 光らせ始めた時刻 ミリ秒 ヒットストップ中も消えていくよう実時間で数える
        int startTime = 0;
    };

    VECTOR _center = VGet(0.0f, 0.0f, 0.0f);
    float _radius = 0.0f;
    Transform* _viewer = nullptr;
    std::vector<VERTEX3D> _vertices;

    Impact _impacts[IMPACT_COUNT];
    int _nextImpact = 0;

public:
    static ArenaBoundary* Get() { return _instance; }

    ~ArenaBoundary() override;

    void Setup(VECTOR center, float radius);

    // 誰に近いところを光らせるか ふつうはプレイヤー
    void SetViewer(Transform* viewer) { _viewer = viewer; }

    // ぶつかった所の幕を一瞬光らせる strength は 0〜1
    void Flash(VECTOR position, float strength);

    void Render() override;

private:
    void AddViewerGlow();
    void AddImpactGlow(const Impact& impact, float fade);

    // 幕の上の点 angle は中心から見た向き
    VECTOR PointAt(float angle, float height) const;

    // 四角を三角 2 枚で足す
    void AddQuad(const VERTEX3D& leftBottom, const VERTEX3D& leftTop, const VERTEX3D& rightBottom, const VERTEX3D& rightTop);
};
