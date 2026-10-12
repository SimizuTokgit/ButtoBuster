#include "MarkerRenderer.h"
#include "Character.h"
#include "CharacterRegistry.h"
#include "StageBuilder.h"
#include "EffectDraw.h"
#include "Transform.h"
#include <cmath>

using namespace EffectDraw;

namespace {
    constexpr const char* ARROW_IMAGE = "Data/Effect/MarkerArrow.png";
    constexpr const char* ENEMY_RING_IMAGE = "Data/Effect/RingEnemy.png";
    constexpr const char* PLAYER_RING_IMAGE = "Data/Effect/RingPlayer.png";
    constexpr const char* ENEMY_RANGE_IMAGE = "Data/Effect/RangeEnemy.png";
    constexpr const char* PLAYER_RANGE_IMAGE = "Data/Effect/RangePlayer.png";

    // ----- 足元の影 -----
    constexpr float SHADOW_SCALE = 2.2f;        // 体の半径の何倍の大きさか
    constexpr float SHADOW_ALPHA = 0.55f;       // 真ん中の濃さ 0〜1 外の縁では 0 になる

    // ----- 足元の輪 (RingEnemy.png RingPlayer.png) -----
    constexpr float RING_SCALE = 2.0f;          // 輪を体の半径の何倍のところに描くか
    constexpr float RING_ALPHA = 0.9f;
    constexpr float RING_IMAGE_RATE = 0.667f;   // 画像の半分の幅に対して、輪のある所 外へは星の飾りが出ている

    // ----- 攻撃範囲 (RangeEnemy.png RangePlayer.png) -----
    constexpr float RANGE_IMAGE_RATE = 0.779f;  // 画像の半分の幅に対して、外側の輪のある所 ここを届く距離に合わせる
    constexpr float RANGE_ALPHA = 0.55f;        // 範囲全体の濃さ
    constexpr float RANGE_FILL_ALPHA = 0.8f;    // 中から満ちていく分の濃さ 満ちきったときに当たる

    // ----- 頭の上の矢印 (MarkerArrow.png) 囲まれていても、どこに敵がいるか分かるように -----
    struct ArrowLook {
        float size;         // 横の幅
        float gap;          // 頭のてっぺんから矢印の真ん中まで
        float bobHeight;    // 上下に揺らす幅
        float bobSpeed;     // 揺らす速さ ラジアン毎秒
    };
    constexpr ArrowLook ARROW = { 55.0f, 70.0f, 8.0f, 4.0f };

    // 地面に埋もれてちらつかないよう少し浮かせる 範囲は足元の印より上に重ねる
    constexpr float LIFT = 3.0f;
    constexpr float RANGE_LIFT = 4.0f;

    // 丸 1 周を何分割で描くか 扇は角度の割合で減らす
    constexpr int SEGMENTS = 32;
}

MarkerRenderer::~MarkerRenderer() {
    for (int graph : { _arrowGraph, _enemyRingGraph, _playerRingGraph, _enemyRangeGraph, _playerRangeGraph }) {
        if (graph != -1) DeleteGraph(graph);
    }
}

void MarkerRenderer::Setup() {
    _arrowGraph = LoadGraph(ARROW_IMAGE);
    _enemyRingGraph = LoadGraph(ENEMY_RING_IMAGE);
    _playerRingGraph = LoadGraph(PLAYER_RING_IMAGE);
    _enemyRangeGraph = LoadGraph(ENEMY_RANGE_IMAGE);
    _playerRangeGraph = LoadGraph(PLAYER_RANGE_IMAGE);

    // 半透明のものと一緒に、不透明なモデルのあとで描く
    renderQueue = RENDER_QUEUE_TRANSPARENT;
    Register();
}

void MarkerRenderer::Render() {
    if (!enabled) {
        _ranges.clear();
        return;
    }

    for (Character* character : CharacterRegistry::GetAll()) {
        if (character && !character->IsDead()) BuildFootMarker(*character);
    }

    for (const RangeDesc& range : _ranges) {
        BuildRange(range, 1.0f, RANGE_ALPHA);
        if (range.fill > 0.0f) BuildRange(range, Mathf::Clamp01(range.fill), RANGE_FILL_ALPHA);
    }
    _ranges.clear();

    // 影は光らせず、普通に重ねる 輪と範囲はその上に
    {
        TransparentScope scope(DX_BLENDMODE_ALPHA);
        DrawBatch(_shadowVertices, DX_NONE_GRAPH);
        DrawBatch(_enemyRingVertices, _enemyRingGraph);
        DrawBatch(_playerRingVertices, _playerRingGraph);
        DrawBatch(_enemyRangeVertices, _enemyRangeGraph);
        DrawBatch(_playerRangeVertices, _playerRangeGraph);
    }

    DrawArrows();
}

void MarkerRenderer::DrawBatch(std::vector<VERTEX3D>& vertices, int graph) {
    if (!vertices.empty()) DrawPolygon3D(vertices.data(), static_cast<int>(vertices.size() / 3), graph, TRUE);
    vertices.clear();
}

void MarkerRenderer::BuildFootMarker(const Character& character) {
    // 地面に立っていれば足の高さ、宙にいれば真下の地面
    VECTOR ground = character.GetPosition();
    if (!character.IsGrounded()) ground = StageBuilder::SnapToGround(ground);

    VECTOR center = VAdd(ground, VGet(0.0f, LIFT, 0.0f));
    float bodyRadius = character.bodyRadius;
    const COLOR_U8 black = GetColorU8(0, 0, 0, 255);
    const COLOR_U8 white = GetColorU8(255, 255, 255, 255);

    // 影 真ん中から外の縁へ薄くなる丸
    float shadowRadius = bodyRadius * SHADOW_SCALE;
    VERTEX3D centerVertex = MakeVertex(center, black, SHADOW_ALPHA);
    for (int i = 0; i < SEGMENTS; ++i) {
        float a0 = DX_TWO_PI_F * i / SEGMENTS;
        float a1 = DX_TWO_PI_F * (i + 1) / SEGMENTS;
        _shadowVertices.push_back(centerVertex);
        _shadowVertices.push_back(MakeVertex(VAdd(center, VGet(cosf(a0) * shadowRadius, 0.0f, sinf(a0) * shadowRadius)), black, 0.0f));
        _shadowVertices.push_back(MakeVertex(VAdd(center, VGet(cosf(a1) * shadowRadius, 0.0f, sinf(a1) * shadowRadius)), black, 0.0f));
    }

    // 輪 画像の輪が体の半径の RING_SCALE 倍に来るよう、画像の板を広げる
    float half = bodyRadius * RING_SCALE / RING_IMAGE_RATE;
    VERTEX3D a0 = MakeVertex(VAdd(center, VGet(-half, 0.0f, half)), white, RING_ALPHA, 0.0f, 0.0f);
    VERTEX3D a1 = MakeVertex(VAdd(center, VGet(half, 0.0f, half)), white, RING_ALPHA, 1.0f, 0.0f);
    VERTEX3D b0 = MakeVertex(VAdd(center, VGet(-half, 0.0f, -half)), white, RING_ALPHA, 0.0f, 1.0f);
    VERTEX3D b1 = MakeVertex(VAdd(center, VGet(half, 0.0f, -half)), white, RING_ALPHA, 1.0f, 1.0f);
    PushQuad(character.team == Team::Enemy ? _enemyRingVertices : _playerRingVertices, a0, a1, b0, b1);
}

void MarkerRenderer::BuildRange(const RangeDesc& desc, float scale, float alpha) {
    if (desc.radius <= 0.0f || scale <= 0.0f) return;

    std::vector<VERTEX3D>& out = desc.isPlayer ? _playerRangeVertices : _enemyRangeVertices;
    const COLOR_U8 white = GetColorU8(255, 255, 255, 255);

    // 画像の外側の輪が届く距離に来るよう、画像の半分の幅をこの長さにする
    // 満ちていく分は画像ごと縮めるので、uv は縮めない
    float imageHalf = desc.radius / RANGE_IMAGE_RATE * scale;
    VECTOR center = VAdd(desc.center, VGet(0.0f, RANGE_LIFT, 0.0f));

    // 扇の真ん中の向きから左右に arcDegree ずつ 画像は世界の向きに固定して、扇の形に切り取る
    float arc = Transform::Deg2Rad(desc.arcDegree > 180.0f ? 180.0f : desc.arcDegree);
    float middle = atan2f(desc.forward.z, desc.forward.x);
    int segments = static_cast<int>(ceilf(SEGMENTS * arc / DX_PI_F));
    if (segments < 1) segments = 1;

    auto edgeVertex = [&](float angle) {
        float x = cosf(angle);
        float z = sinf(angle);
        VECTOR position = VAdd(center, VGet(x * imageHalf, 0.0f, z * imageHalf));
        return MakeVertex(position, white, alpha, 0.5f + x * 0.5f, 0.5f - z * 0.5f);
    };

    VERTEX3D centerVertex = MakeVertex(center, white, alpha, 0.5f, 0.5f);
    for (int i = 0; i < segments; ++i) {
        float a0 = middle - arc + 2.0f * arc * i / segments;
        float a1 = middle - arc + 2.0f * arc * (i + 1) / segments;
        out.push_back(centerVertex);
        out.push_back(edgeVertex(a0));
        out.push_back(edgeVertex(a1));
    }
}

void MarkerRenderer::DrawArrows() {
    if (_arrowGraph == -1) return;

    // 囲まれても見失わないよう、ほかの敵やプレイヤーに隠れずに描く
    SetUseZBufferFlag(FALSE);
    SetUseLighting(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

    float bob = sinf(GetNowCount() / 1000.0f * ARROW.bobSpeed) * ARROW.bobHeight;
    for (Character* character : CharacterRegistry::GetAll()) {
        if (!character || character->team != Team::Enemy || character->IsDead()) continue;

        VECTOR position = VAdd(character->GetTopPosition(), VGet(0.0f, ARROW.gap + bob, 0.0f));
        DrawBillboard3D(position, 0.5f, 0.5f, ARROW.size, 0.0f, _arrowGraph, TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseLighting(TRUE);
}
