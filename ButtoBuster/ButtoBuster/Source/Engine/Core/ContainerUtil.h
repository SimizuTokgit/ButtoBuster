#pragma once
#include <vector>
#include <algorithm>

// マネージャーの登録一覧を出し入れする 同じ書き方を各マネージャーに書かないよう、ここに 1 つだけ置く
namespace ContainerUtil {
    // まだ入っていなければ末尾に足す nullptr は入れない
    template<typename T>
    void AddUnique(std::vector<T*>& list, T* item) {
        if (!item) return;
        if (std::find(list.begin(), list.end(), item) == list.end()) {
            list.push_back(item);
        }
    }

    // 入っていれば外す
    template<typename T>
    void Remove(std::vector<T*>& list, T* item) {
        auto it = std::find(list.begin(), list.end(), item);
        if (it != list.end()) {
            list.erase(it);
        }
    }
}
