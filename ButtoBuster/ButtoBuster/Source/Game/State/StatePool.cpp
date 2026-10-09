#include "StatePool.h"
#include <new>

namespace {
    // 大きさをこの単位で切り上げ、同じ大きさごとに置き場を分ける
    constexpr std::size_t SIZE_STEP = 16;

    // これより大きい状態は置き場を使わず、そのままヒープから取る (今いちばん大きい状態は 200 バイトほど)
    constexpr std::size_t MAX_POOLED_SIZE = 512;

    constexpr std::size_t BUCKET_COUNT = MAX_POOLED_SIZE / SIZE_STEP;

    // 置き場に戻したメモリ 中身はもう使わないので、次の空きを指すのに使う
    struct FreeBlock {
        FreeBlock* next;
    };

    FreeBlock* g_freeLists[BUCKET_COUNT] = {};
    StatePool::Stats g_stats;

    bool IsPooled(std::size_t size) {
        return size > 0 && size <= MAX_POOLED_SIZE;
    }

    std::size_t BucketOf(std::size_t size) {
        return (size + SIZE_STEP - 1) / SIZE_STEP - 1;
    }

    std::size_t RoundedSize(std::size_t size) {
        return (BucketOf(size) + 1) * SIZE_STEP;
    }
}

void* StatePool::Allocate(std::size_t size) {
    g_stats.createdCount++;
    g_stats.aliveCount++;
    g_stats.usedBytes += size;

    if (IsPooled(size)) {
        // 置き場に同じ大きさの空きがあれば、それを使う
        FreeBlock*& freeList = g_freeLists[BucketOf(size)];
        if (freeList) {
            FreeBlock* block = freeList;
            freeList = block->next;
            return block;
        }
        size = RoundedSize(size);
    }

    // 置き場が空のときだけヒープから取る
    g_stats.heapCount++;
    g_stats.heapBytes += size;
    return ::operator new(size);
}

void StatePool::Free(void* memory, std::size_t size) {
    if (!memory) return;

    g_stats.aliveCount--;
    g_stats.usedBytes -= size;

    if (!IsPooled(size)) {
        g_stats.heapBytes -= size;
        ::operator delete(memory);
        return;
    }

    // ヒープへは返さず、置き場に戻す
    FreeBlock* block = static_cast<FreeBlock*>(memory);
    FreeBlock*& freeList = g_freeLists[BucketOf(size)];
    block->next = freeList;
    freeList = block;
}

const StatePool::Stats& StatePool::GetStats() {
    return g_stats;
}
