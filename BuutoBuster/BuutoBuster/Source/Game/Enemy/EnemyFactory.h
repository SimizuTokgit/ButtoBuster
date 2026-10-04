#pragma once
#include "EnemyData.h"
#include "DxLib.h"

class Enemy;
class Character;

// 敵を1体組み立てる
// 体 当たり判定 頭 AI  モデル アニメ 武器
// 種類ごとの違いは EnemyData とアニメの一覧だけで、組み立ての手順は共通
namespace EnemyFactory {
    // id は回り込む位置をばらけさせるのに使う
    Enemy* Create(EnemyKind kind, VECTOR position, Character* target, int id);
}
