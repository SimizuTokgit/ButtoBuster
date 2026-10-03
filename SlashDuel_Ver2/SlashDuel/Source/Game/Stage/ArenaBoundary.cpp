#include "ArenaBoundary.h"
#include "Transform.h"
#include <cmath>

void ArenaBoundary::Setup(VECTOR center, float radius) {
    _center = center;
    _radius = radius;
    renderQueue = RENDER_QUEUE_TRANSPARENT;
    Register();
}

void ArenaBoundary::Render() {
    if (!enabled || !_viewer) return;

    VECTOR viewer = _viewer->position;
    float bottom = viewer.y - WALL_BELOW;
    float top = viewer.y + WALL_ABOVE;

    _vertices.clear();

    auto makeVertex = [](VECTOR position, float alpha) {
        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        vertex.dif = GetColorU8(90, 200, 255, static_cast<int>(alpha * 255.0f));
        vertex.spc = GetColorU8(0, 0, 0, 0);
        return vertex;
    };

    for (int i = 0; i < SEGMENT_COUNT; ++i) {
        float angleA = DX_TWO_PI_F * i / SEGMENT_COUNT;
        float angleB = DX_TWO_PI_F * (i + 1) / SEGMENT_COUNT;

        VECTOR a = VGet(_center.x + cosf(angleA) * _radius, 0.0f, _center.z + sinf(angleA) * _radius);
        VECTOR b = VGet(_center.x + cosf(angleB) * _radius, 0.0f, _center.z + sinf(angleB) * _radius);

        // 幕の近いところほど濃く
        VECTOR middle = VScale(VAdd(a, b), 0.5f);
        float distance = VSize(VGet(middle.x - viewer.x, 0.0f, middle.z - viewer.z));
        float strength = 1.0f - distance / VISIBLE_DISTANCE;
        if (strength <= 0.0f) continue;

        float alpha = strength * 0.55f;

        VERTEX3D aBottom = makeVertex(VGet(a.x, bottom, a.z), alpha);
        VERTEX3D bBottom = makeVertex(VGet(b.x, bottom, b.z), alpha);
        // 上に行くほど消える
        VERTEX3D aTop = makeVertex(VGet(a.x, top, a.z), 0.0f);
        VERTEX3D bTop = makeVertex(VGet(b.x, top, b.z), 0.0f);

        _vertices.push_back(aBottom);
        _vertices.push_back(aTop);
        _vertices.push_back(bBottom);

        _vertices.push_back(bBottom);
        _vertices.push_back(aTop);
        _vertices.push_back(bTop);
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
