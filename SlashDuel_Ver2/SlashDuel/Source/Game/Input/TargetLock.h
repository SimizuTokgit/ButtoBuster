#pragma once
#include "DxLib.h"

class Character;

// ロックオンする相手を選び、持ち続ける
// 無双の ZL と同じく、見ている向きの近くにいる、狙いやすい敵を選ぶ
//
// 相手は倒れたあと少しすると消える 消えた相手のメモリは触れないので、
// 毎フレーム Validate で一覧にまだいるかを確かめてから使う
class TargetLock {
public:
    // 狙える距離
    static constexpr float SEARCH_RANGE = 1800.0f;

    // これより離れたら外れる 狙える距離より長くして、境目で付いたり外れたりしないようにする
    static constexpr float BREAK_RANGE = 2600.0f;

private:
    const Character* _target = nullptr;

public:
    bool HasTarget() const { return _target != nullptr; }
    const Character* GetTarget() const { return _target; }

    // 見ている向きの近くから一番狙いやすい敵を選ぶ 見つからなければ false
    bool Acquire(const Character& owner, VECTOR viewForward);

    // 今の相手の隣へ移す direction は画面の右が 1 左が -1 隣がいなければ false
    bool Switch(const Character& owner, int direction);

    void Release() { _target = nullptr; }

    // 倒れた相手 消えた相手 離れすぎた相手から外す
    void Validate(const Character& owner);

private:
    static bool IsCandidate(const Character& owner, const Character* other);
};
