#pragma once
#include "Observer.h"
#include "ChainEvent.h"
#include "DxLib.h"
#include <memory>
#include <vector>

class Character;

// 1 回の振りから始まった連鎖ぶっ飛ばし
// 吹き飛んでいる敵は砲弾になり、触れた敵を同じ連鎖に巻き込んで吹き飛ばす
// 巻き込まれた敵も砲弾になるので、群れの中を次々に広がっていく
//
// 振った攻撃と、飛んでいる敵がみんなで持ち、最後の 1 人が手放すまで残る
// 何体巻き込んだかは、作るときに渡された Subject で知らせる
class BlowChain : public std::enable_shared_from_this<BlowChain> {
private:
    // これより遅い砲弾は、もう相手を巻き込まない
    static constexpr float MIN_SPEED = 300.0f;

    // 当てた相手に渡す速さの割合と、当てた側に残る速さの割合
    static constexpr float TRANSFER_RATE = 0.8f;
    static constexpr float KEEP_RATE = 0.6f;

    // 重さの比で飛び方を変える幅 重い砲弾は軽い相手を遠くへ、軽い砲弾は重い相手を少しだけ飛ばす
    static constexpr float WEIGHT_RATE_MIN = 0.5f;
    static constexpr float WEIGHT_RATE_MAX = 2.0f;

    // 巻き込んだ相手へのダメージ = 基本 + 速さ × 割合 + 何体目か × 増える分
    static constexpr float BASE_DAMAGE = 8.0f;
    static constexpr float SPEED_DAMAGE_RATE = 0.01f;
    static constexpr float COUNT_DAMAGE = 2.0f;

    // 体どうしが触れたとみなす余白 押し合いで少し離されていても当てる
    // 当たり判定のカプセルより手足や武器が外に出ているので、少し広めにしても当たって見える
    static constexpr float TOUCH_MARGIN = 40.0f;

    Subject<ChainEvent>* _events = nullptr;

    // この連鎖に入った敵 1 体は 1 回しか巻き込まない 跳ね返ってきた砲弾と打ち合いにならないように
    // 中身は読まず、アドレスで比べるだけ
    std::vector<const Character*> _members;

    int _hitCount = 0;
    int _flyingCount = 0;
    bool _hasEnded = false;

public:
    // 作るのは Create から shared_ptr で持つ 砲弾が自分を相手に渡すため
    static std::shared_ptr<BlowChain> Create(Subject<ChainEvent>* events);

    explicit BlowChain(Subject<ChainEvent>* events) : _events(events) {}

    // 砲弾として飛び始めた
    void BeginFlight(const Character& projectile);

    // 着地したり、遅くなったりして砲弾をやめた 全員が止まったら連鎖の終わりを知らせる
    void EndFlight(VECTOR position);

    // 飛んでいる敵が、このフレームに触れた相手を巻き込む
    // まだ相手を巻き込める速さなら true 遅くなったら false
    bool Sweep(Character& projectile, float deltaTime);

    int GetHitCount() const { return _hitCount; }

private:
    bool Contains(const Character* character) const;
    void Notify(ChainEvent::Type type, VECTOR position);
};
