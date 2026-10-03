#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

// 板を組み合わせて描く演出
// 斬撃の弧と、地面に広がる衝撃波の輪
//
// 経過は描く直前に倍率のかかった時間で進める
// ヒットストップやスローの間は、弧も輪も一緒に止まって見える
class ShapeEffectRenderer : public Renderer {
public:
    // 斬撃の弧 振った向きに三日月を描く
    struct ArcDesc {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);
        VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
        float radius = 150.0f;
        float width = 60.0f;

        // 弧の開き 度
        float arcDegree = 150.0f;

        // 0 で水平 90 で縦 振り下ろしや斬り上げを傾きで見せる
        float tiltDegree = 0.0f;

        // 1 で左から右へ -1 で右から左へ 縦のときは 1 で下から上へ
        float swing = 1.0f;

        COLOR_U8 color = GetColorU8(255, 255, 255, 255);
        float life = 0.2f;
    };

    // 地面に広がる輪
    struct RingDesc {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);
        float startRadius = 0.0f;
        float endRadius = 300.0f;
        float width = 60.0f;
        COLOR_U8 color = GetColorU8(255, 255, 255, 255);
        float life = 0.4f;

        // 始めと終わりの濃さ 予兆の輪は濃くなっていき、衝撃波は薄れていく
        float alphaStart = 1.0f;
        float alphaEnd = 0.0f;
    };

private:
    // 同時に出す数の上限 超えたら古いものから消す
    static constexpr int MAX_ARCS = 16;
    static constexpr int MAX_RINGS = 16;

    static constexpr int ARC_SEGMENTS = 20;
    static constexpr int RING_SEGMENTS = 48;

    struct Arc {
        ArcDesc desc;
        float age = 0.0f;
    };

    struct Ring {
        RingDesc desc;
        float age = 0.0f;
    };

    int _arcGraph = -1;
    int _ringGraph = -1;

    std::vector<Arc> _arcs;
    std::vector<Ring> _rings;
    std::vector<VERTEX3D> _vertices;

public:
    ~ShapeEffectRenderer() override;

    void Setup(const char* arcTexture, const char* ringTexture);

    void AddArc(const ArcDesc& desc);
    void AddRing(const RingDesc& desc);

    void Render() override;

private:
    void Advance(float deltaTime);
    void BuildArc(const Arc& arc);
    void BuildRing(const Ring& ring);
    void Flush(int graph);
};
