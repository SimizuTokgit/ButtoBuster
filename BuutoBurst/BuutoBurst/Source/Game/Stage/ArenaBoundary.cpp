#include "ArenaBoundary.h"
#include "Transform.h"
#include <cmath>

namespace {
    VERTEX3D MakeVertex(VECTOR position, COLOR_U8 color, float alpha) {
        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        vertex.dif = GetColorU8(color.r, color.g, color.b, static_cast<int>(alpha * 255.0f));
        vertex.spc = GetColorU8(0, 0, 0, 0);
        return vertex;
    }
}

ArenaBoundary::~ArenaBoundary() {
    if (_instance == this) _instance = nullptr;
}

void ArenaBoundary::Setup(VECTOR center, float radius) {
    _instance = this;
    _center = center;
    _radius = radius;
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

    if (_viewer) AddViewerGlow();

    int now = GetNowCount();
    for (const Impact& impact : _impacts) {
        if (impact.strength <= 0.0f) continue;

        float fade = 1.0f - (now - impact.startTime) / 1000.0f / impact.life;
        if (fade > 0.0f) AddImpactGlow(impact, fade);
    }

    if (_vertices.empty()) return;

    SetUseZBufferFlag(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

    DrawPolygon3D(_vertices.data(), static_cast<int>(_vertices.size() / 3), DX_NONE_GRAPH, TRUE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseLighting(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseZBufferFlag(FALSE);
}

void ArenaBoundary::AddViewerGlow() {
    VECTOR viewer = _viewer->position;
    float bottom = viewer.y - WALL_BELOW;
    float top = viewer.y + WALL_ABOVE;
    COLOR_U8 color = GetColorU8(90, 200, 255, 255);

    for (int i = 0; i < SEGMENT_COUNT; ++i) {
        float angleA = DX_TWO_PI_F * i / SEGMENT_COUNT;
        float angleB = DX_TWO_PI_F * (i + 1) / SEGMENT_COUNT;

        // 幕の近いところほど濃く
        VECTOR middle = VScale(VAdd(PointAt(angleA, 0.0f), PointAt(angleB, 0.0f)), 0.5f);
        float distance = VSize(VGet(middle.x - viewer.x, 0.0f, middle.z - viewer.z));
        float strength = 1.0f - distance / VISIBLE_DISTANCE;
        if (strength <= 0.0f) continue;

        float alpha = strength * 0.55f;

        // 上に行くほど消える
        AddQuad(
            MakeVertex(PointAt(angleA, bottom), color, alpha),
            MakeVertex(PointAt(angleA, top), color, 0.0f),
            MakeVertex(PointAt(angleB, bottom), color, alpha),
            MakeVertex(PointAt(angleB, top), color, 0.0f));
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
        // ぶつかった所がいちばん明るく、左右の端で消える
        float rateA = -1.0f + 2.0f * i / STEPS;
        float rateB = -1.0f + 2.0f * (i + 1) / STEPS;
        float angleA = impact.angle + spread * rateA;
        float angleB = impact.angle + spread * rateB;
        float alphaA = peak * (1.0f - fabsf(rateA));
        float alphaB = peak * (1.0f - fabsf(rateB));

        // ぶつかった高さを明るい芯にして、上下へ消していく
        AddQuad(
            MakeVertex(PointAt(angleA, bottom), color, 0.0f),
            MakeVertex(PointAt(angleA, impact.height), color, alphaA),
            MakeVertex(PointAt(angleB, bottom), color, 0.0f),
            MakeVertex(PointAt(angleB, impact.height), color, alphaB));
        AddQuad(
            MakeVertex(PointAt(angleA, impact.height), color, alphaA),
            MakeVertex(PointAt(angleA, top), color, 0.0f),
            MakeVertex(PointAt(angleB, impact.height), color, alphaB),
            MakeVertex(PointAt(angleB, top), color, 0.0f));
    }
}

VECTOR ArenaBoundary::PointAt(float angle, float height) const {
    return VGet(_center.x + cosf(angle) * _radius, height, _center.z + sinf(angle) * _radius);
}

void ArenaBoundary::AddQuad(const VERTEX3D& leftBottom, const VERTEX3D& leftTop, const VERTEX3D& rightBottom, const VERTEX3D& rightTop) {
    _vertices.push_back(leftBottom);
    _vertices.push_back(leftTop);
    _vertices.push_back(rightBottom);

    _vertices.push_back(rightBottom);
    _vertices.push_back(leftTop);
    _vertices.push_back(rightTop);
}
