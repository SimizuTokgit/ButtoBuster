#pragma once
#include <algorithm>
#include <vector>

class Enemy;

// 攻撃してよい権利
// 無双のように、囲んでいる敵全員が同時に殴ってはこない
// 権利を持った数体だけが攻撃し、残りは間合いを取って回り込む
// これが無いと一斉に殴られて理不尽になり、見た目も団子になる
class AttackTokenPool {
private:
    std::vector<const Enemy*> _holders;
    int _capacity = 1;

public:
    void SetCapacity(int capacity) { _capacity = capacity; }
    int GetCapacity() const { return _capacity; }
    int GetHolderCount() const { return static_cast<int>(_holders.size()); }

    bool TryAcquire(const Enemy* enemy) {
        if (Has(enemy)) return true;
        if (static_cast<int>(_holders.size()) >= _capacity) return false;

        _holders.push_back(enemy);
        return true;
    }

    void Release(const Enemy* enemy) {
        auto it = std::find(_holders.begin(), _holders.end(), enemy);
        if (it != _holders.end()) _holders.erase(it);
    }

    bool Has(const Enemy* enemy) const {
        return std::find(_holders.begin(), _holders.end(), enemy) != _holders.end();
    }

    void Clear() { _holders.clear(); }
};
