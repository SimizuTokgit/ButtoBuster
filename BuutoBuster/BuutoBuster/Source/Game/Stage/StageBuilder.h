#pragma once
#include "DxLib.h"

class Camera;

// ステージを組み立てる
// 地形と空と小物
// 小物の置き場所は、元素材の配置データ Stage00.dat をそのまま読む
namespace StageBuilder {

    // 戦える範囲 ステージ自体はもっと広いが、敵に囲まれる距離に絞る
    constexpr float ARENA_RADIUS = 1600.0f;

    bool Build(Camera* camera);

    // 真上から地面を探して高さを返す 見つからなければ false
    // 敵を地面の上に出すときと、Bee が浮く高さを決めるときに使う
    bool FindGroundHeight(float x, float z, float& outY);

    VECTOR GetArenaCenter();
}
