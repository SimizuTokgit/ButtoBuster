#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

class Transform;

// 戦える範囲の端に壁があることを知らせる光の幕
// 模様の画像 (Data/Effect/ArenaWall.png) の板を、円に沿って一周並べる 画像が読めなければ、下から上へ消えていく色だけの幕
// いつも薄く見せておき、近づいた部分は濃くする 場の端がどこか、どこへ飛ばせば壁に届くかがいつでも分かるように
// 吹き飛んだ体がぶつかった所は、離れていても一瞬光らせる 何もない所で跳ね返ったように見えないように
//
// シーンに 1 つだけ置き、どこからでも Get() で呼べるようにする 壁そのものの決まりは ArenaWall
class ArenaBoundary : public Renderer {
private:
    static inline ArenaBoundary* _instance = nullptr;

    static constexpr int SEGMENT_COUNT = 72;

    // ----- 模様の板 -----
    static constexpr const char* WALL_IMAGE = "Data/Effect/ArenaWall.png";

    // 一周に並べる板の数 区切り (SEGMENT_COUNT) を割り切れる数にする 1 枚が区切り 2 つ分
    // 半径 1600 の円なら、1 枚の幅は約 280
    static constexpr int PANEL_COUNT = 36;
    static_assert(SEGMENT_COUNT % PANEL_COUNT == 0, "PANEL_COUNT は SEGMENT_COUNT を割り切れる数にする");

    // 板の高さと、地面に埋める深さ 画像は 315 x 638 なので、幅 280 なら高さ 565 くらいで縦横の比が合う
    static constexpr float PANEL_HEIGHT = 565.0f;
    static constexpr float PANEL_SINK = 20.0f;

    // 板を縦に何段積むか 1 段だと低く見えるので、上に 2 段重ねて高い壁にする
    static constexpr int PANEL_ROWS = 3;

    // 幕の濃さ 離れていてもこの濃さで見せ、VISIBLE_DISTANCE より近づくほど NEAR_ALPHA へ濃くする
    static constexpr float BASE_ALPHA = 0.35f;
    static constexpr float NEAR_ALPHA = 0.75f;
    static constexpr float VISIBLE_DISTANCE = 700.0f;

    // 色だけの幕の高さ 地面から下と上へどこまで張るか 上へ行くほど消える (画像が読めないとき)
    static constexpr float WALL_BELOW = 200.0f;
    static constexpr float WALL_ABOVE = 450.0f;

    // ぶつかった所を光らせる 同時に覚えておく数と、ふつうの消えるまでの秒数と、左右に広げる角度 (度)
    static constexpr int IMPACT_COUNT = 8;
    static constexpr float IMPACT_TIME = 0.5f;
    static constexpr float IMPACT_SPREAD = 8.0f;

    struct Impact {
        // 中心から見た向き ラジアン
        float angle = 0.0f;
        float height = 0.0f;

        // 0 なら使っていない
        float strength = 0.0f;

        // 消えるまでの秒数と、左右に広げる角度 ラジアン
        float life = IMPACT_TIME;
        float spread = 0.0f;

        // 光らせ始めた時刻 ミリ秒 ヒットストップ中も消えていくよう実時間で数える
        int startTime = 0;
    };

    int _wallGraph = -1;

    VECTOR _center = VGet(0.0f, 0.0f, 0.0f);
    float _radius = 0.0f;
    Transform* _viewer = nullptr;

    // 幕の区切りごとの地面の高さ 幕の足元をここに合わせる 作ったときに一度だけ調べる
    float _groundHeights[SEGMENT_COUNT] = {};
    std::vector<VERTEX3D> _vertices;      // 模様の板 (画像を貼る)
    std::vector<VERTEX3D> _glowVertices;  // 色だけの幕と、ぶつかった所の光

    Impact _impacts[IMPACT_COUNT];
    int _nextImpact = 0;

public:
    static ArenaBoundary* Get() { return _instance; }

    ~ArenaBoundary() override;

    void Setup(VECTOR center, float radius);

    // 誰に近いところを濃くするか ふつうはプレイヤー
    void SetViewer(Transform* viewer) { _viewer = viewer; }

    // ぶつかった所の幕を光らせる strength は 0〜1
    // 壁が割れたときのように大きく長く光らせたいときは、秒数と左右に広げる角度 (度) を渡す
    void Flash(VECTOR position, float strength, float seconds = IMPACT_TIME, float spreadDegree = IMPACT_SPREAD);

    void Render() override;

private:
    // 円の幕を一周ぶん足す 模様の板か、色だけの幕
    void AddPanels();
    void AddCurtain();
    void AddImpactGlow(const Impact& impact, float fade);

    // 見る人に近いところほど濃く angle は中心から見た向き
    float GetAlphaAt(float angleA, float angleB) const;

    // 幕の上の点 angle は中心から見た向き
    VECTOR PointAt(float angle, float height) const;

    // 四角を三角 2 枚で足す
    static void AddQuad(std::vector<VERTEX3D>& out,
        const VERTEX3D& leftBottom, const VERTEX3D& leftTop, const VERTEX3D& rightBottom, const VERTEX3D& rightTop);
};
