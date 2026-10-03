#pragma once
#include "MonoBehaviour.h"
#include "CapsuleCollider.h"

// 当たり判定を目で見るための部品 F2 で出る
// 押し合う体は緑、すり抜ける体 倒れた敵 は灰色で出す
class ColliderGizmo : public MonoBehaviour {
private:
    CapsuleCollider* _capsule = nullptr;

public:
    void Start() override {
        _capsule = GetComponent<CapsuleCollider>();
    }

    void OnDrawGizmos() override {
        if (!_capsule) return;

        unsigned int color = _capsule->isTrigger ? 0x808080 : 0x00FF00;
        _capsule->DrawGizmo(color);
    }
};
