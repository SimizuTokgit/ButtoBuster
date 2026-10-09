#pragma once
#include "InputInfo.h"
#include "StatePool.h"
#include <cstddef>

// 状態の基底
// Player と Enemy の両方で使うのでテンプレートにしてある
// T は状態を持つ本人の型
template<class T>
class ICharacterState {
public:
    // 基底のポインタのまま消すので virtual にしておく
    // 付け忘れると派生側のデストラクタが呼ばれない
    virtual ~ICharacterState() = default;

    // 状態は切り替えのたびに作って消すので、メモリは StatePool の置き場から取る
    // 派生した状態はこれを受け継ぐので、状態のクラスは make_unique で作るだけでよい
    // デストラクタが virtual なので、消すときの size には派生した状態の大きさが来る
    static void* operator new(std::size_t size) { return StatePool::Allocate(size); }
    static void operator delete(void* memory, std::size_t size) { StatePool::Free(memory, size); }

    virtual void Enter(T& owner) {}
    virtual void Execute(T& owner, const InputInfo& input, float deltaTime) {}
    virtual void Exit(T& owner) {}

    // デバッグ表示に出す名前
    virtual const char* GetName() const = 0;

    // あと何秒、回避もできないか 敵の AI が隙を狙うときに読む
    // 隙の無い状態は書かなくてよい 0 が返る
    virtual float GetOpeningTime(const T& owner) const { return 0.0f; }
};
