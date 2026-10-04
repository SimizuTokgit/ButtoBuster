#pragma once
#include "Observer.h"
#include "WallBreakEvent.h"
#include "DxLib.h"

class Character;

// 戦える範囲の端にある壁
// プレイヤーも敵も、この円の外へは出られない
// 吹き飛んだ勢いのままぶつかると跳ね返る (壁バウンド) 飛ばした側へ戻ってくるので、追い打ちにつながる
// 速くぶつかった敵は、戻ってくる間も連鎖の砲弾のまま 戻る速さが砲弾の速さを下回れば、ただ跳ね返るだけ
//
// 吹っ飛ばされ値が許容値に届いていれば、跳ね返らずに壁を割って場外へ飛ぶ (壁割り) 敵なら撃破、プレイヤーなら負け
// 壁にぶつけた回数は数えない 割れるかどうかは、ぶつかった瞬間の吹っ飛ばされ値だけで決まる
//
// 円の中心と半径は StageBuilder の戦える範囲を使い、見た目の光の幕は ArenaBoundary が描く
namespace ArenaWall {
    // 壁へ向かう速さがこれより遅ければ跳ね返らず、壁際で止まる 押されて少し滑っただけで跳ねないように
    // 吹き飛んだ敵が砲弾でなくなる速さ (BlowChain の MIN_SPEED) とそろえてある 割れるのもこの速さから
    constexpr float BOUNCE_MIN_SPEED = 300.0f;

    // 跳ね返る速さ = 壁へ向かっていた速さ × これ 1 なら同じ速さのまま返る
    constexpr float BOUNCE_RATE = 0.7f;

    // 跳ね返るときに上へ跳ねる速さ 壁の足元に落ちず、宙を戻ってくるのが見えるように
    constexpr float BOUNCE_JUMP_SPEED = 400.0f;

    // この速さでぶつかったときに、揺れ 火花 光をいちばん強くする
    constexpr float FULL_IMPACT_SPEED = 1200.0f;

    // 割ったときに場外へ飛んでいく速さ = 壁へ向かっていた速さ × これ 遅くても下の速さでは飛ばす
    constexpr float BREAK_FLY_RATE = 1.2f;
    constexpr float BREAK_FLY_MIN_SPEED = 1500.0f;

    // 壁にぶつかったときの様子
    struct Hit {
        // 壁に触れた場所 高さはぶつかった体の足元
        VECTOR point = VGet(0.0f, 0.0f, 0.0f);

        // 壁から内側へ向かう水平の向き 跳ね返る向き
        VECTOR normal = VGet(0.0f, 0.0f, 0.0f);

        // 壁へ向かっていた水平の速さ
        float speed = 0.0f;
    };

    // 吹き飛んでぶつかった結果
    enum class Reaction {
        None,       // 勢いが足りず、壁際で止まった
        Bounce,     // 跳ね返った
        Break,      // 壁を割って場外へ飛んだ
    };

    // 体を壁の内側へ戻し、壁へ向かう速さを消す 物理で動いたあとに毎フレーム呼ぶ
    // 壁へ向かって動いていたら、ぶつかった様子を outHit に入れて true
    bool KeepInside(Character& character, Hit& outHit);

    // 吹き飛んでいる体が壁にぶつかったときに呼ぶ
    // 勢いが足りれば、吹っ飛ばされ値が許容値に届いているときは壁を割り、届いていなければ跳ね返す
    // 割ったときは、体を Defeat で場外へ飛ばしてから知らせを出す 演出はその知らせを受けた Observer が出す
    // 跳ね返ったときの速さと手応え (火花 揺れ 音 光の幕) はここで出す 向きやアニメは呼んだ状態が決める
    Reaction React(Character& character, const Hit& hit);

    // 壁が割れたことを知らせる先 演出を出す Observer はここに登録する
    Subject<WallBreakEvent>& GetBreakEvents();

    // 位置が壁の外なら、壁から margin だけ内側へ寄せて返す 敵が向かう先を決めるときに使う
    VECTOR ClampInside(VECTOR position, float margin);
}
