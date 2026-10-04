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
};
