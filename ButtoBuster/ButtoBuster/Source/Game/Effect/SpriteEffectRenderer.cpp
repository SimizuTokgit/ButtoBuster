#include "SpriteEffectRenderer.h"
#include "EffectDraw.h"
#include "Time.h"
#include "Mathf.h"

using namespace EffectDraw;

namespace {
    const VECTOR UP = { 0.0f, 1.0f, 0.0f };
}

void SpriteEffectRenderer::Setup() {
    renderQueue = RENDER_QUEUE_TRANSPARENT;
    Register();
}

void SpriteEffectRenderer::Add(const SpriteDesc& desc) {
    if (desc.life <= 0.0f || desc.width <= 0.0f || desc.height <= 0.0f) return;
    if (static_cast<int>(_sprites.size()) >= MAX_SPRITES) _sprites.erase(_sprites.begin());

    Sprite sprite;
    sprite.desc = desc;
    _sprites.push_back(sprite);
}

void SpriteEffectRenderer::Render() {
    Advance(Time::DeltaTime());
    if (!enabled || _sprites.empty()) return;

    // 混ぜ方は板ごとに Draw で決める
    TransparentScope scope;

    VECTOR cameraPosition = GetCameraPosition();
    for (const auto& sprite : _sprites) Draw(sprite, cameraPosition);
}

void SpriteEffectRenderer::Advance(float deltaTime) {
    AdvanceAndRemove(_sprites, deltaTime);
}

void SpriteEffectRenderer::Draw(const Sprite& sprite, VECTOR cameraPosition) {
    const SpriteDesc& desc = sprite.desc;

    // 画像が読めなかったときは出さない 色だけの四角が出てしまうので
    if (desc.graph == -1) return;

    float progress = sprite.age / desc.life;

    // fadeStart までは始めの濃さのまま、そこから終わりの濃さへ
    float alpha = desc.alphaStart;
    if (progress > desc.fadeStart && desc.fadeStart < 1.0f) {
        float fade = (progress - desc.fadeStart) / (1.0f - desc.fadeStart);
        alpha = desc.alphaStart + (desc.alphaEnd - desc.alphaStart) * fade;
    }
    if (desc.flicker > 0.0f) {
        alpha *= 1.0f - desc.flicker * Mathf::RandomRange(0.0f, 1.0f);
    }
    if (alpha <= 0.0f) return;

    // 出た瞬間の大きさから、勢いよく広がって元の大きさになる
    float scale = 1.0f;
    if (desc.growTime > 0.0f && sprite.age < desc.growTime) {
        float eased = Mathf::EaseOutQuad(sprite.age / desc.growTime);
        scale = desc.startScale + (1.0f - desc.startScale) * eased;
    }
    float width = desc.width * scale;
    float height = desc.height * scale;

    // 板の表の向き 渡されていなければカメラのほうへ向ける 縦はまっすぐのまま
    VECTOR facing = desc.facing;
    facing.y = 0.0f;
    if (VSquareSize(facing) < 0.0001f) {
        facing = VSub(cameraPosition, desc.position);
        facing.y = 0.0f;
        if (VSquareSize(facing) < 0.0001f) facing = VGet(0.0f, 0.0f, -1.0f);
    }
    facing = VNorm(facing);

    // 表から見たときの右 見ている向き (表の逆) と上から決める
    VECTOR right = VNorm(VCross(UP, VScale(facing, -1.0f)));

    VECTOR halfWidth = VScale(right, width * 0.5f);
    VECTOR bottom = VSub(desc.position, VScale(UP, height * desc.anchorY));
    VECTOR top = VAdd(bottom, VScale(UP, height));

    VERTEX3D vertices[6] = {
        MakeVertex(VSub(top, halfWidth), desc.color, alpha, 0.0f, 0.0f),
        MakeVertex(VAdd(top, halfWidth), desc.color, alpha, 1.0f, 0.0f),
        MakeVertex(VSub(bottom, halfWidth), desc.color, alpha, 0.0f, 1.0f),

        MakeVertex(VSub(bottom, halfWidth), desc.color, alpha, 0.0f, 1.0f),
        MakeVertex(VAdd(top, halfWidth), desc.color, alpha, 1.0f, 0.0f),
        MakeVertex(VAdd(bottom, halfWidth), desc.color, alpha, 1.0f, 1.0f),
    };

    SetDrawBlendMode(desc.blendMode, 255);
    DrawPolygon3D(vertices, 2, desc.graph, TRUE);
}
