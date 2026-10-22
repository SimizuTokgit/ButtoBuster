#include "ShapeEffectRenderer.h"
#include "EffectDraw.h"
#include "Transform.h"
#include "Time.h"
#include <cmath>

using namespace EffectDraw;

namespace {
    // 弧が振り終わりまで伸びきるまでの割合 残りの時間で薄れて消える
    constexpr float ARC_REVEAL = 0.35f;

    // 消えるまでに弧が外へ広がる割合 止まった絵に見えないように
    constexpr float ARC_GROW = 0.12f;

    // 弧の両端の細り方 1 より小さいほど真ん中の太い所が長く続く
    constexpr float ARC_TAPER_POWER = 0.6f;

    // 通り過ぎたところの濃さ 振っている先は 1
    constexpr float ARC_TAIL_ALPHA = 0.35f;

    // 地面に埋もれてちらつかないよう少し浮かせる
    constexpr float RING_LIFT = 4.0f;

    // 輪のいちばん濃いところ 外の縁から太さのこの割合だけ内側
    constexpr float RING_PEAK_RATE = 0.3f;
}

ShapeEffectRenderer::~ShapeEffectRenderer() {
    if (_arcGraph != -1) DeleteGraph(_arcGraph);
    if (_ringGraph != -1) DeleteGraph(_ringGraph);
}

void ShapeEffectRenderer::Setup(const char* arcTexture, const char* ringTexture) {
    _arcGraph = LoadGraph(arcTexture);
    _ringGraph = LoadGraph(ringTexture);

    renderQueue = RENDER_QUEUE_TRANSPARENT;
    Register();
}

void ShapeEffectRenderer::AddArc(const ArcDesc& desc) {
    if (desc.life <= 0.0f || desc.radius <= 0.0f) return;
    if (static_cast<int>(_arcs.size()) >= MAX_ARCS) _arcs.erase(_arcs.begin());

    Arc arc;
    arc.desc = desc;
    _arcs.push_back(arc);
}

void ShapeEffectRenderer::AddRing(const RingDesc& desc) {
    if (desc.life <= 0.0f) return;
    if (static_cast<int>(_rings.size()) >= MAX_RINGS) _rings.erase(_rings.begin());

    Ring ring;
    ring.desc = desc;
    _rings.push_back(ring);
}

void ShapeEffectRenderer::Render() {
    Advance(Time::DeltaTime());
    if (!enabled) return;
    if (_arcs.empty() && _rings.empty()) return;

    TransparentScope scope(DX_BLENDMODE_ADD);

    _vertices.clear();
    for (const auto& ring : _rings) BuildRing(ring);
    Flush(_ringGraph);

    _vertices.clear();
    for (const auto& arc : _arcs) BuildArc(arc);
    Flush(_arcGraph);
}

void ShapeEffectRenderer::Advance(float deltaTime) {
    AdvanceAndRemove(_arcs, deltaTime);
    AdvanceAndRemove(_rings, deltaTime);
}

void ShapeEffectRenderer::BuildArc(const Arc& arc) {
    const ArcDesc& desc = arc.desc;
    float progress = arc.age / desc.life;

    // 振った先へ伸びていき、伸びきったら全体が薄れる
    float reach = Mathf::Clamp01(progress / ARC_REVEAL);
    float fade = (progress < ARC_REVEAL) ? 1.0f : 1.0f - (progress - ARC_REVEAL) / (1.0f - ARC_REVEAL);
    if (reach <= 0.0f || fade <= 0.0f) return;

    VECTOR forward = desc.forward;
    forward.y = 0.0f;
    if (VSquareSize(forward) < 0.0001f) return;
    forward = VNorm(forward);

    VECTOR up = VGet(0.0f, 1.0f, 0.0f);
    VECTOR right = VNorm(VCross(up, forward));

    // 傾けた面の中で弧を描く 横の向きを上へ倒すほど縦の振りになる
    float tilt = Transform::Deg2Rad(desc.tiltDegree);
    VECTOR side = VAdd(VScale(right, cosf(tilt)), VScale(up, sinf(tilt)));

    float halfArc = Transform::Deg2Rad(desc.arcDegree * 0.5f);
    float radius = desc.radius * (1.0f + ARC_GROW * progress);

    VERTEX3D prevOuter{};
    VERTEX3D prevInner{};
    for (int i = 0; i <= ARC_SEGMENTS; ++i) {
        // 0 が振り始め 1 が振り終わり
        float along = reach * i / ARC_SEGMENTS;
        float angle = (-halfArc + 2.0f * halfArc * along) * desc.swing;
        VECTOR direction = VAdd(VScale(forward, cosf(angle)), VScale(side, sinf(angle)));

        // 両端を細くして三日月にする
        // 端では sin がわずかに負になり、powf が数でなくなるので 0 で止める
        float taper = Mathf::Clamp01(sinf(DX_PI_F * along));
        float width = desc.width * powf(taper, ARC_TAPER_POWER);

        // 振っている先ほど濃く、通り過ぎたところほど薄く
        float trail = (reach > 0.0f) ? along / reach : 0.0f;
        float alpha = fade * (ARC_TAIL_ALPHA + (1.0f - ARC_TAIL_ALPHA) * trail);

        VECTOR outer = VAdd(desc.center, VScale(direction, radius));
        VECTOR inner = VAdd(desc.center, VScale(direction, radius - width));

        // 画像は上の端が濃いので、外側を上に合わせて刃先を光らせる
        VERTEX3D outerVertex = MakeVertex(outer, desc.color, alpha, along, 0.0f);
        VERTEX3D innerVertex = MakeVertex(inner, desc.color, alpha, along, 1.0f);

        if (i > 0) PushQuad(_vertices, prevOuter, prevInner, outerVertex, innerVertex);
        prevOuter = outerVertex;
        prevInner = innerVertex;
    }
}

void ShapeEffectRenderer::BuildRing(const Ring& ring) {
    const RingDesc& desc = ring.desc;
    float progress = ring.age / desc.life;

    // 最初に勢いよく広がり、だんだんゆっくりになる
    float radius = desc.startRadius + (desc.endRadius - desc.startRadius) * Mathf::EaseOutQuad(progress);
    float alpha = desc.alphaStart + (desc.alphaEnd - desc.alphaStart) * progress;
    if (radius <= 0.0f || alpha <= 0.0f) return;

    // 外の縁を一番濃くして、内と外へぼかす
    float outerRadius = radius;
    float peakRadius = radius - desc.width * RING_PEAK_RATE;
    if (peakRadius < 0.0f) peakRadius = 0.0f;
    float innerRadius = radius - desc.width;
    if (innerRadius < 0.0f) innerRadius = 0.0f;

    VECTOR center = VAdd(desc.center, VGet(0.0f, RING_LIFT, 0.0f));

    VERTEX3D prev[3]{};
    for (int i = 0; i <= RING_SEGMENTS; ++i) {
        float t = static_cast<float>(i) / RING_SEGMENTS;
        float angle = DX_TWO_PI_F * t;
        VECTOR direction = VGet(cosf(angle), 0.0f, sinf(angle));

        // 画像を繰り返すと貼り方の設定に左右されるので、1 周で 1 枚にする
        float u = t;

        VERTEX3D current[3] = {
            MakeVertex(VAdd(center, VScale(direction, innerRadius)), desc.color, 0.0f, u, 1.0f),
            MakeVertex(VAdd(center, VScale(direction, peakRadius)), desc.color, alpha, u, 0.5f),
            MakeVertex(VAdd(center, VScale(direction, outerRadius)), desc.color, 0.0f, u, 0.0f),
        };

        if (i > 0) {
            PushQuad(_vertices, prev[0], prev[1], current[0], current[1]);
            PushQuad(_vertices, prev[1], prev[2], current[1], current[2]);
        }
        for (int k = 0; k < 3; ++k) prev[k] = current[k];
    }
}

void ShapeEffectRenderer::Flush(int graph) {
    if (_vertices.empty()) return;

    // 画像が読めなかったときは色だけで描く
    int handle = (graph != -1) ? graph : DX_NONE_GRAPH;
    DrawPolygon3D(_vertices.data(), static_cast<int>(_vertices.size() / 3), handle, TRUE);
}
