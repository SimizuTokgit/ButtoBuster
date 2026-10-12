#pragma once
#include "EnemyData.h"
#include "DxLib.h"

class Enemy;
class Character;

// 敵を1体組み立てる
// 体 当たり判定 頭 AI  モデル アニメ 武器
// 種類ごとの違いは EnemyData とアニメの一覧だけで、組み立ての手順は共通
namespace EnemyFactory {
    // id は敵ごとに動く時をずらすのに使う 浮いて上下に揺れる時 (Enemy) と、囲んで左右へ踏み替える時 (EnemyAI の Encircle)
    Enemy* Create(EnemyKind kind, VECTOR position, Character* target, int id);

    // 数値を書き換えた敵を出すとき (チュートリアルの練習の相手) アニメと音は data.kind の種類のものを使う
    // 敵は data を指したまま持つので、data は敵が消えるまで残しておくこと
    Enemy* Create(const EnemyData& data, VECTOR position, Character* target, int id);
}
