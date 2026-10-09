#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

// 画像を 1 枚の板に貼って出す演出
// 必殺技の雷と、壁が割れたときのヒビ
//
// 板は縦にまっすぐ立てたまま、向きだけを決める
// facing を渡せばその向きに固定し (壁のヒビ)、渡さなければカメラのほうを向く (雷)
//
// 経過は ShapeEffectRenderer と同じく、描く直前に倍率のかかった時間で進める
// ヒットストップやスローの間は、板も一緒に止まって見える
class SpriteEffectRenderer : public Renderer {
public:
    struct SpriteDesc {
        // 画像 EffectManager が読んだものを入れる
        int graph = -1;

        // 板を置く場所 anchorY で決めた高さの、横の真ん中がここに来る
        VECTOR position = VGet(0.0f, 0.0f, 0.0f);

        // 0 で板の下端を position に置く 0.5 で真ん中、1 で上端
        float anchorY = 0.0f;

        // 板の表が向く向き 水平だけ見る 長さ 0 ならカメラのほうを向く
        VECTOR facing = VGet(0.0f, 0.0f, 0.0f);

        float width = 300.0f;
        float height = 300.0f;

        COLOR_U8 color = GetColorU8(255, 255, 255, 255);

        // 出ている秒数
        float life = 0.3f;

        // 濃さは fadeStart (life に対する割合) までは alphaStart のまま、そこから life の終わりで alphaEnd になる
        float alphaStart = 1.0f;
        float alphaEnd = 0.0f;
        float fadeStart = 0.0f;

        // 濃さのちらつき 0 でちらつかない 1 で毎フレーム 0〜1 倍に揺れる 雷の瞬きに使う
        float flicker = 0.0f;

        // 出た瞬間の大きさ (1 で最初から同じ大きさ) と、元の大きさになるまでの秒数
        // ヒビが一気に広がって見えるように
        float startScale = 1.0f;
        float growTime = 0.0f;

        // DX_BLENDMODE_ADD で光らせる (雷) DX_BLENDMODE_ALPHA でそのまま重ねる (ヒビ)
        int blendMode = DX_BLENDMODE_ADD;
    };

private:
    // 同時に出す数の上限 超えたら古いものから消す 雷は敵の数だけ出る
    static constexpr int MAX_SPRITES = 32;

    struct Sprite {
        SpriteDesc desc;
        float age = 0.0f;
    };

    std::vector<Sprite> _sprites;

public:
    void Setup();

    void Add(const SpriteDesc& desc);

    void Render() override;

private:
    void Advance(float deltaTime);
    void Draw(const Sprite& sprite, VECTOR cameraPosition);
};
