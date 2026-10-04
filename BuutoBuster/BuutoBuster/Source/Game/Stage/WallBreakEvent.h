#pragma once
#include "DxLib.h"

class Character;

// 壁が割れたことを知らせる中身
// 吹っ飛ばされ値が許容値に届いた体が勢いよく壁にぶつかると、跳ね返らずに壁を割って場外へ飛ぶ
// 敵なら撃破、プレイヤーなら負け
//
// 音 エフェクト 画面の演出は、これを受け取った Observer がそれぞれ出す
// 壁を割った側は、誰が受け取っているかを知らない
struct WallBreakEvent {
    // 割った体 中身は読むだけにする このあと場外へ飛んで消えていく
    const Character* character = nullptr;
    bool isPlayer = false;

    // 割れた場所 ぶつかった体の胴の高さ
    VECTOR position = VGet(0.0f, 0.0f, 0.0f);

    // 壁から内側への向き 体はこの逆へ飛んでいく
    VECTOR normal = VGet(0.0f, 0.0f, 0.0f);

    // 壁へ向かっていた速さ 速いほど派手にしてよい
    float speed = 0.0f;
};
