#include "EnemyMarkerRenderer.h"
#include "Character.h"
#include "CharacterRegistry.h"
#include "StageBuilder.h"
#include <cmath>

namespace {
    // ----- 影 -----
    constexpr float SHADOW_SCALE = 2.2f;        // 体の半径の何倍の大きさか
    constexpr float SHADOW_ALPHA = 0.55f;       // 真ん中の濃さ 0〜1 外の縁では 0 になる

    // ----- 輪 (影の縁) -----
    constexpr float RING_SCALE = 2.0f;          // 体の半径の何倍のところに描くか
    constexpr float RING_WIDTH = 10.0f;         // 太さ 内と外へぼかす
    constexpr float RING_ALPHA = 0.85f;         // いちばん濃いところの濃さ 0〜1
    const COLOR_U8 RING_COLOR = GetColorU8(255, 80, 60, 255);   // 敵だと分かる赤

    // 地面に埋もれてちらつかないよう少し浮かせる
    constexpr float LIFT = 3.0f;

    constexpr int SEGMENTS = 32;

    VERTEX3D MakeVertex(VECTOR position, COLOR_U8 color, float alpha) {
        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        vertex.dif = color;
        vertex.dif.a = static_cast<unsigned char>(alpha * 255.0f);
        vertex.spc = GetColorU8(0, 0, 0, 0);
        return vertex;
    }
}

void EnemyMarkerRenderer::Setup() {
    // 半透明のものと一緒に、不透明なモデルのあとで描く
    renderQueue = RENDER_QUEUE_TRANSPARENT;
    Register();
}

void EnemyMarkerRenderer::Render() {
    if (!enabled) return;

    _vertices.clear();
    for (Character* character : CharacterRegistry::GetAll()) {
        if (!character || character->team != Team::Enemy || character->IsDead()) continue;

        // 地面に立っていれば足の高さ、宙にいれば真下の地面
        VECTOR ground = character->GetPosition();
        if (!character->IsGrounded()) StageBuilder::FindGroundHeight(ground.x, ground.z, ground.y);
        BuildMarker(ground, character->bodyRadius);
    }
    if (_vertices.empty()) return;

    // 奥のモデルには隠れるが、自分は奥行きを書かない 重なった印どうしが欠けないように
    SetUseZBufferFlag(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

    DrawPolygon3D(_vertices.data(), static_cast<int>(_vertices.size() / 3), DX_NONE_GRAPH, TRUE);

    // ほかのエフェクトと同じ初期の状態に戻す
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseLighting(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseZBufferFlag(FALSE);
}

void EnemyMarkerRenderer::BuildMarker(VECTOR ground, float bodyRadius) {
    VECTOR center = VAdd(ground, VGet(0.0f, LIFT, 0.0f));
    const COLOR_U8 black = GetColorU8(0, 0, 0, 255);

    float shadowRadius = bodyRadius * SHADOW_SCALE;
    float ringRadius = bodyRadius * RING_SCALE;
    float ringInner = ringRadius - RING_WIDTH;
    float ringOuter = ringRadius + RING_WIDTH;

    VERTEX3D centerVertex = MakeVertex(center, black, SHADOW_ALPHA);
    for (int i = 0; i < SEGMENTS; ++i) {
        float a0 = DX_TWO_PI_F * i / SEGMENTS;
        float a1 = DX_TWO_PI_F * (i + 1) / SEGMENTS;
        VECTOR d0 = VGet(cosf(a0), 0.0f, sinf(a0));
        VECTOR d1 = VGet(cosf(a1), 0.0f, sinf(a1));

        // 影 真ん中から外の縁へ薄くなる扇
        _vertices.push_back(centerVertex);
        _vertices.push_back(MakeVertex(VAdd(center, VScale(d0, shadowRadius)), black, 0.0f));
        _vertices.push_back(MakeVertex(VAdd(center, VScale(d1, shadowRadius)), black, 0.0f));

        // 輪 真ん中の線がいちばん濃く、内と外へぼかす
        VERTEX3D in0 = MakeVertex(VAdd(center, VScale(d0, ringInner)), RING_COLOR, 0.0f);
        VERTEX3D in1 = MakeVertex(VAdd(center, VScale(d1, ringInner)), RING_COLOR, 0.0f);
        VERTEX3D mid0 = MakeVertex(VAdd(center, VScale(d0, ringRadius)), RING_COLOR, RING_ALPHA);
        VERTEX3D mid1 = MakeVertex(VAdd(center, VScale(d1, ringRadius)), RING_COLOR, RING_ALPHA);
        VERTEX3D out0 = MakeVertex(VAdd(center, VScale(d0, ringOuter)), RING_COLOR, 0.0f);
        VERTEX3D out1 = MakeVertex(VAdd(center, VScale(d1, ringOuter)), RING_COLOR, 0.0f);

        const VERTEX3D quads[2][4] = { { in0, mid0, in1, mid1 }, { mid0, out0, mid1, out1 } };
        for (const auto& q : quads) {
            _vertices.push_back(q[0]); _vertices.push_back(q[1]); _vertices.push_back(q[2]);
            _vertices.push_back(q[2]); _vertices.push_back(q[1]); _vertices.push_back(q[3]);
        }
    }
}
