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
    constexpr const char* RANGE_IMAGE = "Data/Effect/RangeEnemy.png";

    // ----- 足元の影 -----
    constexpr float SHADOW_SCALE = 2.2f;        // 体の半径の何倍の大きさか
    constexpr float SHADOW_ALPHA = 0.55f;       // 真ん中の濃さ 0〜1 外の縁では 0 になる

    // ----- 足元の輪 (RingEnemy.png RingPlayer.png) -----
    constexpr float RING_SCALE = 2.0f;          // 輪を体の半径の何倍のところに描くか
    constexpr float RING_ALPHA = 0.9f;
    constexpr float RING_IMAGE_RATE = 0.667f;   // 画像の半分の幅に対して、輪のある所 外へは星の飾りが出ている

    // ----- 攻撃範囲 (RangeEnemy.png) -----
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

    // 地面に沿わせる網目の細かさ 輪は GRID x GRID のます、範囲は中心から外へ GRID 段に分ける
    // 頂点ごとに真下の地面の高さを測るので、細かくするほど沿うが、測る回数が増える
    constexpr int GRID = 4;

    // (x, z) の真下の地面から lift だけ浮かせた所 地面が見つからなければ fallbackY の高さに置く
    VECTOR OnGround(float x, float z, float fallbackY, float lift) {
        float y = fallbackY;
        StageBuilder::FindGroundHeight(x, z, y);
        return VGet(x, y + lift, z);
    }
}

MarkerRenderer::~MarkerRenderer() {
    for (int graph : { _arrowGraph, _enemyRingGraph, _playerRingGraph, _rangeGraph }) {
        if (graph != -1) DeleteGraph(graph);
    }
}

void MarkerRenderer::Setup() {
    _arrowGraph = LoadGraph(ARROW_IMAGE);
    _enemyRingGraph = LoadGraph(ENEMY_RING_IMAGE);
    _playerRingGraph = LoadGraph(PLAYER_RING_IMAGE);
    _rangeGraph = LoadGraph(RANGE_IMAGE);

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
        DrawBatch(_rangeVertices, _rangeGraph);
    }

    DrawArrows();
}

void MarkerRenderer::DrawBatch(std::vector<VERTEX3D>& vertices, int graph) {
    if (!vertices.empty()) DrawPolygon3D(vertices.data(), static_cast<int>(vertices.size() / 3), graph, TRUE);
    vertices.clear();
}

void MarkerRenderer::BuildFootMarker(const Character& character) {
    // 頂点ごとに真下の地面の高さを測るので、宙にいても足元の地面に描ける
    VECTOR foot = character.GetPosition();
    float bodyRadius = character.bodyRadius;
    const COLOR_U8 black = GetColorU8(0, 0, 0, 255);
    const COLOR_U8 white = GetColorU8(255, 255, 255, 255);

    // 影 真ん中から外の縁へ薄くなる丸
    float shadowRadius = bodyRadius * SHADOW_SCALE;
    auto shadowEdge = [&](int i) {
        float angle = DX_TWO_PI_F * i / SEGMENTS;
        VECTOR position = OnGround(foot.x + cosf(angle) * shadowRadius, foot.z + sinf(angle) * shadowRadius, foot.y, LIFT);
        return MakeVertex(position, black, 0.0f);
    };
    VERTEX3D centerVertex = MakeVertex(OnGround(foot.x, foot.z, foot.y, LIFT), black, SHADOW_ALPHA);
    VERTEX3D previous = shadowEdge(0);
    for (int i = 1; i <= SEGMENTS; ++i) {
        VERTEX3D next = shadowEdge(i);
        _shadowVertices.push_back(centerVertex);
        _shadowVertices.push_back(previous);
        _shadowVertices.push_back(next);
        previous = next;
    }

    // 輪 画像の輪が体の半径の RING_SCALE 倍に来るよう、画像の板を広げる 板は GRID x GRID のますに分けて地面に沿わせる
    float half = bodyRadius * RING_SCALE / RING_IMAGE_RATE;
    VERTEX3D grid[GRID + 1][GRID + 1];
    for (int row = 0; row <= GRID; ++row) {
        for (int column = 0; column <= GRID; ++column) {
            float u = static_cast<float>(column) / GRID;
            float v = static_cast<float>(row) / GRID;
            VECTOR position = OnGround(foot.x - half + 2.0f * half * u, foot.z + half - 2.0f * half * v, foot.y, LIFT);
            grid[row][column] = MakeVertex(position, white, RING_ALPHA, u, v);
        }
    }

    std::vector<VERTEX3D>& out = (character.team == Team::Enemy) ? _enemyRingVertices : _playerRingVertices;
    for (int row = 0; row < GRID; ++row) {
        for (int column = 0; column < GRID; ++column) {
            PushQuad(out, grid[row][column], grid[row][column + 1], grid[row + 1][column], grid[row + 1][column + 1]);
        }
    }
}

void MarkerRenderer::BuildRange(const RangeDesc& desc, float scale, float alpha) {
    if (desc.radius <= 0.0f || scale <= 0.0f) return;

    const COLOR_U8 white = GetColorU8(255, 255, 255, 255);

    // 画像の外側の輪が届く距離に来るよう、画像の半分の幅をこの長さにする
    // 満ちていく分は画像ごと縮めるので、uv は縮めない
    float imageHalf = desc.radius / RANGE_IMAGE_RATE * scale;
    VECTOR center = desc.center;

    // 扇の真ん中の向きから左右に arcDegree ずつ 画像は世界の向きに固定して、扇の形に切り取る
    float arc = Transform::Deg2Rad(desc.arcDegree > 180.0f ? 180.0f : desc.arcDegree);
    float middle = atan2f(desc.forward.z, desc.forward.x);
    int segments = static_cast<int>(ceilf(SEGMENTS * arc / DX_PI_F));
    if (segments < 1) segments = 1;

    // 中心から外へ GRID 段に分け、頂点ごとに地面に沿わせる rate は中心からの距離の割合 0〜1
    auto rangeVertex = [&](float rate, float angle) {
        float x = cosf(angle) * rate;
        float z = sinf(angle) * rate;
        VECTOR position = OnGround(center.x + x * imageHalf, center.z + z * imageHalf, center.y, RANGE_LIFT);
        return MakeVertex(position, white, alpha, 0.5f + x * 0.5f, 0.5f - z * 0.5f);
    };

    VERTEX3D centerVertex = rangeVertex(0.0f, 0.0f);
    std::vector<VERTEX3D> inner;
    for (int ring = 1; ring <= GRID; ++ring) {
        float rate = static_cast<float>(ring) / GRID;
        std::vector<VERTEX3D> outer;
        for (int i = 0; i <= segments; ++i) {
            outer.push_back(rangeVertex(rate, middle - arc + 2.0f * arc * i / segments));
        }

        for (int i = 0; i < segments; ++i) {
            if (inner.empty()) {
                _rangeVertices.push_back(centerVertex);
                _rangeVertices.push_back(outer[i]);
                _rangeVertices.push_back(outer[i + 1]);
            }
            else {
                PushQuad(_rangeVertices, inner[i], outer[i], inner[i + 1], outer[i + 1]);
            }
        }
        inner = outer;
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
