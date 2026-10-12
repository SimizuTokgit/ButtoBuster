#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

class Character;

// 場所と攻撃範囲を見せる印
// 敵 毎フレーム全員の足元に影と赤い輪を、頭の上に赤い矢印を描く 地面の色に敵が溶けて見えにくいので
//    飛んでいる敵や吹き飛んで宙にいる敵は、足元の印を真下の地面に描く 倒された敵には描かない
// 攻撃範囲 振りかぶっている間、攻撃する側が毎フレーム ShowRange で頼み、頼まれたフレームだけ描く
//    途中で斬られて振りをやめれば、頼まれなくなって消える 敵は赤、プレイヤーは青
//    扇形の技は、丸い画像を扇の形に切り取って描く
// 画像と大きさの数値は MarkerRenderer.cpp の先頭
class MarkerRenderer : public Renderer {
public:
    // 地面に出す攻撃範囲 1 つ分
    struct RangeDesc {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);     // 足元の地面
        VECTOR forward = VGet(0.0f, 0.0f, 1.0f);    // 扇の真ん中の向き
        float radius = 0.0f;                        // 届く距離
        float arcDegree = 180.0f;                   // 正面から左右に何度まで (AttackData と同じ) 180 で全方向
        bool isPlayer = false;                      // 青 (プレイヤーの技) か 赤 (敵の技) か
        float fill = 0.0f;                          // 当たるまでの進み具合 0〜1 中から満ちていく 0 なら満ちる分は描かない
    };

private:
    int _arrowGraph = -1;
    int _ringGraph = -1;
    int _enemyRangeGraph = -1;
    int _playerRangeGraph = -1;

    // 頼まれた攻撃範囲 描いたら空にする
    std::vector<RangeDesc> _ranges;

    // 画像ごとに分けて、まとめて描く
    std::vector<VERTEX3D> _shadowVertices;
    std::vector<VERTEX3D> _ringVertices;
    std::vector<VERTEX3D> _enemyRangeVertices;
    std::vector<VERTEX3D> _playerRangeVertices;

public:
    ~MarkerRenderer() override;

    void Setup();
    void Render() override;

    // このフレームに攻撃範囲を出す
    void ShowRange(const RangeDesc& desc) { _ranges.push_back(desc); }

private:
    void BuildFootMarker(VECTOR ground, float bodyRadius);
    void BuildRange(const RangeDesc& desc, float scale, float alpha);
    void DrawArrows();
    void DrawBatch(std::vector<VERTEX3D>& vertices, int graph);
};
