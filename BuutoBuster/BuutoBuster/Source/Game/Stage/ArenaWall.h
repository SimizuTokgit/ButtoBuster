#pragma once
#include "DxLib.h"

class Character;

// 戦える範囲の端にある壁
// プレイヤーも敵も、この円の外へは出られない
// 吹き飛んだ勢いのままぶつかると跳ね返る (壁バウンド) 飛ばした側へ戻ってくるので、追い打ちにつながる
// 速くぶつかった敵は、戻ってくる間も連鎖の砲弾のまま 戻る速さが砲弾の速さを下回れば、ただ跳ね返るだけ
//
// 円の中心と半径は StageBuilder の戦える範囲を使い、見た目の光の幕は ArenaBoundary が描く
namespace ArenaWall {
    // 壁へ向かう速さがこれより遅ければ跳ね返らず、壁際で止まる 押されて少し滑っただけで跳ねないように
    // 吹き飛んだ敵が砲弾でなくなる速さ (BlowChain の MIN_SPEED) とそろえてある
    constexpr float BOUNCE_MIN_SPEED = 300.0f;

    // 跳ね返る速さ = 壁へ向かっていた速さ × これ 1 なら同じ速さのまま返る
    constexpr float BOUNCE_RATE = 0.7f;

    // 跳ね返るときに上へ跳ねる速さ 壁の足元に落ちず、宙を戻ってくるのが見えるように
    constexpr float BOUNCE_JUMP_SPEED = 400.0f;

    // この速さでぶつかったときに、揺れ 火花 光をいちばん強くする
    constexpr float FULL_IMPACT_SPEED = 1200.0f;

    // 壁にぶつかったときの様子
    struct Hit {
        // 壁に触れた場所 高さはぶつかった体の足元
        VECTOR point = VGet(0.0f, 0.0f, 0.0f);

        // 壁から内側へ向かう水平の向き 跳ね返る向き
        VECTOR normal = VGet(0.0f, 0.0f, 0.0f);

        // 壁へ向かっていた水平の速さ
        float speed = 0.0f;
    };

    // 体を壁の内側へ戻し、壁へ向かう速さを消す 物理で動いたあとに毎フレーム呼ぶ
    // 壁へ向かって動いていたら、ぶつかった様子を outHit に入れて true
    bool KeepInside(Character& character, Hit& outHit);

    // 吹き飛んでいる体が壁にぶつかったときに呼ぶ 勢いが足りれば跳ね返して true
    // 跳ねる速さと手応え (火花 揺れ 音 光の幕) はここで出す 向きやアニメは呼んだ状態が決める
    bool TryBounce(Character& character, const Hit& hit);

    // 位置が壁の外なら、壁から margin だけ内側へ寄せて返す 敵が向かう先を決めるときに使う
    VECTOR ClampInside(VECTOR position, float margin);
}
