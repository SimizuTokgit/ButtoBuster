#pragma once
#include "DxLib.h"
#include "Mathf.h"
#include <algorithm>
#include <vector>

// 板を組み合わせて描く 3D の演出で、同じ書き方をファイルごとに書かないためのもの
// 頂点の作り方 四角の足し方 半透明の板を描く間の設定 経過の進め方
namespace EffectDraw {
    // 板の頂点 濃さ alpha は 0〜1 で、はみ出た分は収める
    // 光を当てないので法線は上向きで決めておく
    inline VERTEX3D MakeVertex(VECTOR position, COLOR_U8 color, float alpha, float u = 0.0f, float v = 0.0f) {
        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        vertex.dif = color;
        vertex.dif.a = static_cast<unsigned char>(Mathf::Clamp01(alpha) * 255.0f);
        vertex.spc = GetColorU8(0, 0, 0, 0);
        vertex.u = u;
        vertex.v = v;
        return vertex;
    }

    // 四角を三角 2 枚で足す a0 a1 が片方の辺 b0 b1 がもう片方の辺
    inline void PushQuad(std::vector<VERTEX3D>& out,
        const VERTEX3D& a0, const VERTEX3D& a1, const VERTEX3D& b0, const VERTEX3D& b1) {
        out.push_back(a0);
        out.push_back(a1);
        out.push_back(b0);

        out.push_back(b0);
        out.push_back(a1);
        out.push_back(b1);
    }

    // 半透明の板を描く間の設定 作ったときに変えて、なくなるときにほかのエフェクトと同じ初期の状態に戻す
    // 奥のモデルには隠れるが、自分は奥行きを書かない 重なった板どうしが欠けないように
    class TransparentScope {
    public:
        // 混ぜ方は板ごとに描く側で決める
        TransparentScope() {
            SetUseZBufferFlag(TRUE);
            SetWriteZBufferFlag(FALSE);
            SetUseLighting(FALSE);
            SetUseBackCulling(FALSE);
        }

        // 全部の板を同じ混ぜ方で描く
        explicit TransparentScope(int blendMode) : TransparentScope() {
            SetDrawBlendMode(blendMode, 255);
        }

        ~TransparentScope() {
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            SetUseLighting(TRUE);
            SetWriteZBufferFlag(FALSE);
            SetUseZBufferFlag(FALSE);
        }

        TransparentScope(const TransparentScope&) = delete;
        TransparentScope& operator=(const TransparentScope&) = delete;
    };

    // 出ているものの経過を進め、出ている時間 (desc.life) を過ぎたものを消す
    // T は age と desc.life を持つもの
    template <class T>
    void AdvanceAndRemove(std::vector<T>& items, float deltaTime) {
        for (auto& item : items) item.age += deltaTime;

        items.erase(
            std::remove_if(items.begin(), items.end(),
                [](const T& item) { return item.age >= item.desc.life; }),
            items.end());
    }
}
