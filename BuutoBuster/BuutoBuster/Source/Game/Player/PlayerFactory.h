#pragma once
#include "DxLib.h"

class Player;

// プレイヤーを1体組み立てる
// 体 当たり判定 操作役 モデル アニメ 剣 鞘 剣の軌跡
namespace PlayerFactory {
    Player* Create(VECTOR position);
}
