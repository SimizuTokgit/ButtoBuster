#include "ShapeEffectRenderer.h"
#include "Time.h"
#include <algorithm>
#include <cmath>

namespace {
    // 弧が振り終わりまで伸びきるまでの割合 残りの時間で薄れて消える
    constexpr float ARC_REVEAL = 0.35f;

    // 消えるまでに弧が外へ広がる割合 止まった絵に見えないように
    constexpr float ARC_GROW = 0.12f;

    // 地面に埋もれてちらつかないよう少し浮かせる
    constexpr float RING_LIFT = 4.0f;

    VERTEX3D MakeVertex(VECTOR position, float u, float v, COLOR_U8 color, float alpha) {
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        vertex.dif = color;
        vertex.dif.a = static_cast<unsigned char>(alpha * 255.0f);
        vertex.spc = GetColorU8(0, 0, 0, 0);
        vertex.u = u;
        vertex.v = v;
        return vertex;
    }

    void PushQuad(std::vector<VERTEX3D>& out,
        const VERTEX3D& a0, const VERTEX3D& a1, const VERTEX3D& b0, const VERTEX3D& b1) {
        out.push_back(a0);
        out.push_back(a1);
        out.push_back(b0);

        out.push_back(b0);
        out.push_back(a1);
        out.push_back(b1);
    }
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

    // 奥のモデルには隠れるが、自分は奥行きを書かない 重なった部分が欠けないように
    SetUseZBufferFlag(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255);

    _vertices.clear();
    for (const auto& ring : _rings) BuildRing(ring);
    Flush(_ringGraph);

    _vertices.clear();
    for (const auto& arc : _arcs) BuildArc(arc);
    Flush(_arcGraph);

    // ほかのエフェクトと同じ初期の状態に戻す
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseLighting(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseZBufferFlag(FALSE);
}

void ShapeEffectRenderer::Advance(float deltaTime) {
    for (auto& arc : _arcs) arc.age += deltaTime;
    for (auto& ring : _rings) ring.age += deltaTime;

    _arcs.erase(
        std::remove_if(_arcs.begin(), _arcs.end(),
            [](const Arc& arc) { return arc.age >= arc.desc.life; }),
        _arcs.end());

    _rings.erase(
        std::remove_if(_rings.begin(), _rings.end(),
            [](const Ring& ring) { return ring.age >= ring.desc.life; }),
        _rings.end());
}

void ShapeEffectRenderer::BuildArc(const Arc& arc) {
    const ArcDesc& desc = arc.desc;
    float progress = arc.age / desc.life;

    // 振った先へ伸びていき、伸びきったら全体が薄れる
    // std::min と std::max は Windows.h のマクロとぶつかるので比べて書く
    float reach = progress / ARC_REVEAL;
    if (reach > 1.0f) reach = 1.0f;
    float fade = (progress < ARC_REVEAL) ? 1.0f : 1.0f - (progress - ARC_REVEAL) / (1.0f - ARC_REVEAL);
    if (reach <= 0.0f || fade <= 0.0f) return;

    VECTOR forward = desc.forward;
    forward.y = 0.0f;
    if (VSquareSize(forward) < 0.0001f) return;
    forward = VNorm(forward);

    VECTOR up = VGet(0.0f, 1.0f, 0.0f);
    VECTOR right = VNorm(VCross(up, forward));

    // 傾けた面の中で弧を描く 横の向きを上へ倒すほど縦の振りになる
    float tilt = desc.tiltDegree * DX_PI_F / 180.0f;
    VECTOR side = VAdd(VScale(right, cosf(tilt)), VScale(up, sinf(tilt)));

    float halfArc = desc.arcDegree * 0.5f * DX_PI_F / 180.0f;
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
        float taper = sinf(DX_PI_F * along);
        if (taper < 0.0f) taper = 0.0f;
        float width = desc.width * powf(taper, 0.6f);

        // 振っている先ほど濃く、通り過ぎたところほど薄く
        float trail = (reach > 0.0f) ? along / reach : 0.0f;
        float alpha = fade * (0.35f + 0.65f * trail);

        VECTOR outer = VAdd(desc.center, VScale(direction, radius));
        VECTOR inner = VAdd(desc.center, VScale(direction, radius - width));

        // 画像は上の端が濃いので、外側を上に合わせて刃先を光らせる
        VERTEX3D outerVertex = MakeVertex(outer, along, 0.0f, desc.color, alpha);
        VERTEX3D innerVertex = MakeVertex(inner, along, 1.0f, desc.color, alpha);

        if (i > 0) PushQuad(_vertices, prevOuter, prevInner, outerVertex, innerVertex);
        prevOuter = outerVertex;
        prevInner = innerVertex;
    }
}

void ShapeEffectRenderer::BuildRing(const Ring& ring) {
    const RingDesc& desc = ring.desc;
    float progress = ring.age / desc.life;

    // 最初に勢いよく広がり、だんだんゆっくりになる
    float eased = 1.0f - (1.0f - progress) * (1.0f - progress);
    float radius = desc.startRadius + (desc.endRadius - desc.startRadius) * eased;
    float alpha = desc.alphaStart + (desc.alphaEnd - desc.alphaStart) * progress;
    if (radius <= 0.0f || alpha <= 0.0f) return;

    // 外の縁を一番濃くして、内と外へぼかす
    float outerRadius = radius;
    float peakRadius = radius - desc.width * 0.3f;
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
            MakeVertex(VAdd(center, VScale(direction, innerRadius)), u, 1.0f, desc.color, 0.0f),
            MakeVertex(VAdd(center, VScale(direction, peakRadius)), u, 0.5f, desc.color, alpha),
            MakeVertex(VAdd(center, VScale(direction, outerRadius)), u, 0.0f, desc.color, 0.0f),
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
