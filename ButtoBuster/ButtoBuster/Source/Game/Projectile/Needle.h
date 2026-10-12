#pragma once
#include "MonoBehaviour.h"
#include "DxLib.h"

class MeshRenderer;

// Bee が撃ってくる針
// 撃つたびに作って消すのではなく、NeedlePool が持っているものを使い回す
class Needle : public MonoBehaviour {
private:
    static constexpr float SPEED = 1100.0f;
    static constexpr float LIFE_TIME = 3.0f;
    static constexpr float HIT_RADIUS = 18.0f;
    static constexpr float KNOCKBACK = 150.0f;

    // 当たった所から、飛んできた道をこの秒数だけ戻った所を、撃った所とみなす ガードの向きの判定に使う
    static constexpr float SOURCE_LOOKBACK_TIME = 0.2f;

    MeshRenderer* _renderer = nullptr;
    VECTOR _velocity = VGet(0.0f, 0.0f, 0.0f);
    float _life = 0.0f;
    int _damage = 0;
    const char* _hitSound = "";
    bool _isFlying = false;

public:
    void SetRenderer(MeshRenderer* renderer) { _renderer = renderer; }

    // hitSound は当たった相手が鳴らす音
    void Launch(VECTOR position, VECTOR direction, int damage, const char* hitSound);

    // GameObject::SetActive は描画までは止めないので、見た目も自分で消す
    void Deactivate();

    bool IsFlying() const { return _isFlying; }

    void Update(float deltaTime) override;

private:
    bool TryHitPlayer(VECTOR position);

    // 針の時間の進み方 撃った敵と同じく、プレイヤーに時間を遅くされている間はゆっくり飛ぶ
    float GetTimeScale() const;
};
