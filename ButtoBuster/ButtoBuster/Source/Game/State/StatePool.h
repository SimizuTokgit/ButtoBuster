#pragma once
#include <cstddef>

// 状態 (ICharacterState) のメモリの置き場
//
// 状態は切り替えのたびに作って消すので、そのたびにヒープから取ると回数がとても多くなる
// 消した状態のメモリは、大きさごとの置き場に戻しておき、次に同じ大きさの状態を作るときに使い回す
// ヒープから取るのは、置き場が空のとき (遊び始めのうち) だけ
//
// 状態のクラスは書き換えなくてよい 状態の元 (ICharacterState) の operator new / delete がここを使う
// 数えた回数は F1 のデバッグ表示に出す (DebugCheats)
namespace StatePool {
    void* Allocate(std::size_t size);
    void Free(void* memory, std::size_t size);

    struct Stats {
        long long createdCount = 0;     // 状態を作った回数 置き場が無ければ、この回数だけヒープから取っていた
        long long heapCount = 0;        // そのうち、ヒープから取った回数
        std::size_t heapBytes = 0;      // ヒープから取ったメモリの合計 (置き場の大きさ)
        std::size_t usedBytes = 0;      // 今使っている状態のメモリの合計
        int aliveCount = 0;             // 今ある状態の数
    };
    const Stats& GetStats();
}
