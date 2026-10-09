#include "SpriteEffectRenderer.h"
#include "Time.h"
#include <algorithm>

namespace {
    const VECTOR UP = { 0.0f, 1.0f, 0.0f };

    VERTEX3D MakeVertex(VECTOR position, float u, float v, COLOR_U8 color, float alpha) {
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = UP;
        vertex.dif = color;
        vertex.dif.a = static_cast<unsigned char>(alpha * 255.0f);
        vertex.spc = GetColorU8(0, 0, 0, 0);
        vertex.u = u;
        vertex.v = v;
        return vertex;
    }
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

    // 奥のモデルには隠れるが、自分は奥行きを書かない 重なった板が欠けないように
    SetUseZBufferFlag(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);

    VECTOR cameraPosition = GetCameraPosition();
    for (const auto& sprite : _sprites) Draw(sprite, cameraPosition);

    // ほかのエフェクトと同じ初期の状態に戻す
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseLighting(TRUE);
    SetWriteZBufferFlag(FALSE);
    SetUseZBufferFlag(FALSE);
}

void SpriteEffectRenderer::Advance(float deltaTime) {
    for (auto& sprite : _sprites) sprite.age += deltaTime;

    _sprites.erase(
        std::remove_if(_sprites.begin(), _sprites.end(),
            [](const Sprite& sprite) { return sprite.age >= sprite.desc.life; }),
        _sprites.end());
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
        alpha *= 1.0f - desc.flicker * (GetRand(1000) / 1000.0f);
    }
    if (alpha <= 0.0f) return;

    // 出た瞬間の大きさから、勢いよく広がって元の大きさになる
    float scale = 1.0f;
    if (desc.growTime > 0.0f && sprite.age < desc.growTime) {
        float t = sprite.age / desc.growTime;
        float eased = 1.0f - (1.0f - t) * (1.0f - t);
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
        MakeVertex(VSub(top, halfWidth), 0.0f, 0.0f, desc.color, alpha),
        MakeVertex(VAdd(top, halfWidth), 1.0f, 0.0f, desc.color, alpha),
        MakeVertex(VSub(bottom, halfWidth), 0.0f, 1.0f, desc.color, alpha),

        MakeVertex(VSub(bottom, halfWidth), 0.0f, 1.0f, desc.color, alpha),
        MakeVertex(VAdd(top, halfWidth), 1.0f, 0.0f, desc.color, alpha),
        MakeVertex(VAdd(bottom, halfWidth), 1.0f, 1.0f, desc.color, alpha),
    };

    SetDrawBlendMode(desc.blendMode, 255);
    DrawPolygon3D(vertices, 2, desc.graph, TRUE);
}
