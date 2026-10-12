#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

class Character;

// 場所と攻撃範囲を見せる印
// 足元 毎フレーム、敵全員とプレイヤーの足元に影と輪を描く 敵は赤、プレイヤーは青 地面の色に溶けて見えにくいので
//    飛んでいる敵や吹き飛んで宙にいるときは、真下の地面に描く 倒されたら描かない
// 頭の上 敵全員に赤い矢印を描く 囲まれていても、どこに敵がいるか分かるように
// 攻撃範囲 振りかぶっている間、攻撃する側が毎フレーム ShowRange で頼み、頼まれたフレームだけ描く
//    途中で斬られて振りをやめれば、頼まれなくなって消える 敵の技だけで、赤で描く
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
        float fill = 0.0f;                          // 当たるまでの進み具合 0〜1 中から満ちていく 0 なら満ちる分は描かない
    };

private:
    int _arrowGraph = -1;
    int _enemyRingGraph = -1;
    int _playerRingGraph = -1;
    int _rangeGraph = -1;

    // 頼まれた攻撃範囲 描いたら空にする
    std::vector<RangeDesc> _ranges;

    // 画像ごとに分けて、まとめて描く
    std::vector<VERTEX3D> _shadowVertices;
    std::vector<VERTEX3D> _enemyRingVertices;
    std::vector<VERTEX3D> _playerRingVertices;
    std::vector<VERTEX3D> _rangeVertices;

public:
    ~MarkerRenderer() override;

    void Setup();
    void Render() override;

    // このフレームに攻撃範囲を出す
    void ShowRange(const RangeDesc& desc) { _ranges.push_back(desc); }

private:
    void BuildFootMarker(const Character& character);
    void BuildRange(const RangeDesc& desc, float scale, float alpha);
    void DrawArrows();
    void DrawBatch(std::vector<VERTEX3D>& vertices, int graph);
};
