#pragma once
#include "InputInfo.h"

// 状態の基底
// Player と Enemy の両方で使うのでテンプレートにしてある
// T は状態を持つ本人の型
template<class T>
class ICharacterState {
public:
    // 基底のポインタのまま消すので virtual にしておく
    // 付け忘れると派生側のデストラクタが呼ばれない
    virtual ~ICharacterState() = default;

    virtual void Enter(T& owner) {}
    virtual void Execute(T& owner, const InputInfo& input, float deltaTime) {}
    virtual void Exit(T& owner) {}

    // デバッグ表示に出す名前
    virtual const char* GetName() const = 0;

    // あと何秒、回避もできないか 敵の AI が隙を狙うときに読む
    // 隙の無い状態は書かなくてよい 0 が返る
    virtual float GetOpeningTime(const T& owner) const { return 0.0f; }
};
