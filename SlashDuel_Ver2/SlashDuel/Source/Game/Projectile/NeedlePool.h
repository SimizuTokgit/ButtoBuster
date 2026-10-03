#pragma once
#include "MonoBehaviour.h"
#include "DxLib.h"
#include <vector>

class Needle;

// 針を使い回す入れ物 オブジェクトプール
//
// 撃つたびに GameObject を作って消すと、モデルの複製と破棄が毎回走って重い
// 最初にまとめて作っておき、飛んでいないものを探して撃つ
// パーティクルの粒が死んだ枠を使い回すのと同じ考え方
class NeedlePool : public MonoBehaviour {
private:
    static inline NeedlePool* _instance = nullptr;

    std::vector<Needle*> _needles;

public:
    static NeedlePool* Get() { return _instance; }

    ~NeedlePool() override;

    void Initialize(int count);

    // 空きが無ければ撃たない 同時に飛ぶ数の上限はここで決まる
    bool Fire(VECTOR position, VECTOR direction, int damage);

    int GetFlyingCount() const;
    int GetCapacity() const { return static_cast<int>(_needles.size()); }
};
