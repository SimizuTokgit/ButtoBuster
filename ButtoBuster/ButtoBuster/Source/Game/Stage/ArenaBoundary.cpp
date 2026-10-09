#include "ArenaBoundary.h"
#include "StageBuilder.h"
#include "Transform.h"
#include <cmath>

namespace {
    VERTEX3D MakeVertex(VECTOR position, COLOR_U8 color, float alpha, float u = 0.0f, float v = 0.0f) {
        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        vertex.dif = GetColorU8(color.r, color.g, color.b, static_cast<int>(alpha * 255.0f));
        vertex.spc = GetColorU8(0, 0, 0, 0);
        vertex.u = u;
        vertex.v = v;
        return vertex;
    }
}

ArenaBoundary::~ArenaBoundary() {
    if (_instance == this) _instance = nullptr;
    if (_wallGraph != -1) DeleteGraph(_wallGraph);
}

void ArenaBoundary::Setup(VECTOR center, float radius) {
    _instance = this;
    _center = center;
    _radius = radius;

    // 幕の足元を地面に合わせる 地面が見つからなければ中心の高さ
    for (int i = 0; i < SEGMENT_COUNT; ++i) {
        VECTOR point = PointAt(DX_TWO_PI_F * i / SEGMENT_COUNT, center.y);
        float groundY = center.y;
        StageBuilder::FindGroundHeight(point.x, point.z, groundY);
        _groundHeights[i] = groundY;
    }

    _wallGraph = LoadGraph(WALL_IMAGE);

    renderQueue = RENDER_QUEUE_TRANSPARENT;
    Register();
}

void ArenaBoundary::Flash(VECTOR position, float strength, float seconds, float spreadDegree) {
    if (seconds <= 0.0f) return;

    // いちばん古いものから上書きする
    Impact& impact = _impacts[_nextImpact];
    _nextImpact = (_nextImpact + 1) % IMPACT_COUNT;

    impact.angle = atan2f(position.z - _center.z, position.x - _center.x);
    impact.height = position.y;
    impact.strength = strength;
    impact.life = seconds;
    impact.spread = spreadDegree * DX_PI_F / 180.0f;
    impact.startTime = GetNowCount();
}

void ArenaBoundary::Render() {
    if (!enabled) return;

    _vertices.clear();
    _glowVertices.clear();

    if (_wallGraph != -1) AddPanels();
    else AddCurtain();

    int now = GetNowCount();
    for (const Impact& impact : _impacts) {
        if (impact.strength <= 0.0f) continue;

        float fade = 1.0f - (now - impact.startTime) / 1000.0f / impact.life;
        if (fade > 0.0f) AddImpactGlow(impact, fade);
    }

    if (_vertices.empty() && _glowVertices.empty()) return;

    SetUseZBufferFlag(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

    if (!_vertices.empty()) {
        DrawPolygon3D(_vertices.data(), static_cast<int>(_vertices.size() / 3), _wallGraph, TRUE);
    }
    if (!_glowVertices.empty()) {
        DrawPolygon3D(_glowVertices.data(), static_cast<int>(_glowVertices.size() / 3), DX_NONE_GRAPH, TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseLighting(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseZBufferFlag(FALSE);
}

void ArenaBoundary::AddPanels() {
    // 画像の色をそのまま使う 濃さだけを見る人との距離で変える
    COLOR_U8 color = GetColorU8(255, 255, 255, 255);
    constexpr int SEGMENTS_PER_PANEL = SEGMENT_COUNT / PANEL_COUNT;

    for (int i = 0; i < SEGMENT_COUNT; ++i) {
        int next = (i + 1) % SEGMENT_COUNT;
        float angleA = DX_TWO_PI_F * i / SEGMENT_COUNT;
        float angleB = DX_TWO_PI_F * (i + 1) / SEGMENT_COUNT;
        float alpha = GetAlphaAt(angleA, angleB);

        // 1 枚の板を区切り SEGMENTS_PER_PANEL 個に分けて貼る 画像の横の位置は板の中のどこか
        float uA = static_cast<float>(i % SEGMENTS_PER_PANEL) / SEGMENTS_PER_PANEL;
        float uB = static_cast<float>(i % SEGMENTS_PER_PANEL + 1) / SEGMENTS_PER_PANEL;

        float bottomA = _groundHeights[i] - PANEL_SINK;
        float bottomB = _groundHeights[next] - PANEL_SINK;
        AddQuad(_vertices,
            MakeVertex(PointAt(angleA, bottomA), color, alpha, uA, 1.0f),
            MakeVertex(PointAt(angleA, bottomA + PANEL_HEIGHT), color, alpha, uA, 0.0f),
            MakeVertex(PointAt(angleB, bottomB), color, alpha, uB, 1.0f),
            MakeVertex(PointAt(angleB, bottomB + PANEL_HEIGHT), color, alpha, uB, 0.0f));
    }
}

void ArenaBoundary::AddCurtain() {
    COLOR_U8 color = GetColorU8(90, 200, 255, 255);

    for (int i = 0; i < SEGMENT_COUNT; ++i) {
        int next = (i + 1) % SEGMENT_COUNT;
        float angleA = DX_TWO_PI_F * i / SEGMENT_COUNT;
        float angleB = DX_TWO_PI_F * (i + 1) / SEGMENT_COUNT;
        float alpha = GetAlphaAt(angleA, angleB);

        // 上に行くほど消える
        float groundA = _groundHeights[i];
        float groundB = _groundHeights[next];
        AddQuad(_glowVertices,
            MakeVertex(PointAt(angleA, groundA - WALL_BELOW), color, alpha),
            MakeVertex(PointAt(angleA, groundA + WALL_ABOVE), color, 0.0f),
            MakeVertex(PointAt(angleB, groundB - WALL_BELOW), color, alpha),
            MakeVertex(PointAt(angleB, groundB + WALL_ABOVE), color, 0.0f));
    }
}

void ArenaBoundary::AddImpactGlow(const Impact& impact, float fade) {
    constexpr int STEPS = 8;
    constexpr float BELOW = 250.0f;
    constexpr float ABOVE = 550.0f;

    // 近づいたときの幕より白に寄せて、ぶつかった瞬間を目立たせる
    COLOR_U8 color = GetColorU8(170, 230, 255, 255);

    float spread = impact.spread;
    float bottom = impact.height - BELOW;
    float top = impact.height + ABOVE;
    float peak = impact.strength * fade;

    for (int i = 0; i < STEPS; ++i) {
        // ぶつかった所がいちばん明るく、左右の端で消える 模様の板の上に、色だけの光として重ねる
        float rateA = -1.0f + 2.0f * i / STEPS;
        float rateB = -1.0f + 2.0f * (i + 1) / STEPS;
        float angleA = impact.angle + spread * rateA;
        float angleB = impact.angle + spread * rateB;
        float alphaA = peak * (1.0f - fabsf(rateA));
        float alphaB = peak * (1.0f - fabsf(rateB));

        // ぶつかった高さを明るい芯にして、上下へ消していく
        AddQuad(_glowVertices,
            MakeVertex(PointAt(angleA, bottom), color, 0.0f),
            MakeVertex(PointAt(angleA, impact.height), color, alphaA),
            MakeVertex(PointAt(angleB, bottom), color, 0.0f),
            MakeVertex(PointAt(angleB, impact.height), color, alphaB));
        AddQuad(_glowVertices,
            MakeVertex(PointAt(angleA, impact.height), color, alphaA),
            MakeVertex(PointAt(angleA, top), color, 0.0f),
            MakeVertex(PointAt(angleB, impact.height), color, alphaB),
            MakeVertex(PointAt(angleB, top), color, 0.0f));
    }
}

float ArenaBoundary::GetAlphaAt(float angleA, float angleB) const {
    // いつも薄く見せ、見る人に近いところほど濃く
    float alpha = BASE_ALPHA;
    if (!_viewer) return alpha;

    VECTOR viewer = _viewer->position;
    VECTOR middle = VScale(VAdd(PointAt(angleA, 0.0f), PointAt(angleB, 0.0f)), 0.5f);
    float distance = VSize(VGet(middle.x - viewer.x, 0.0f, middle.z - viewer.z));
    float closeness = 1.0f - distance / VISIBLE_DISTANCE;
    if (closeness > 0.0f) alpha += (NEAR_ALPHA - BASE_ALPHA) * closeness;
    return alpha;
}

VECTOR ArenaBoundary::PointAt(float angle, float height) const {
    return VGet(_center.x + cosf(angle) * _radius, height, _center.z + sinf(angle) * _radius);
}

void ArenaBoundary::AddQuad(std::vector<VERTEX3D>& out,
    const VERTEX3D& leftBottom, const VERTEX3D& leftTop, const VERTEX3D& rightBottom, const VERTEX3D& rightTop) {
    out.push_back(leftBottom);
    out.push_back(leftTop);
    out.push_back(rightBottom);

    out.push_back(rightBottom);
    out.push_back(leftTop);
    out.push_back(rightTop);
}
