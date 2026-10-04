#pragma once
#include "ICharacterState.h"
#include <memory>
#include <utility>
#include <vector>

// 今の状態を1つ持ち、切り替えを受け付ける
//
// 切り替えはその場では行わず、今の状態の処理が終わってから行う
// 実行中の状態を消すと、その関数の残りが消えたメモリの上で動いてしまう
//
// 同じフレームに切り替えの要求が2つ来たら、先に来たほうを採る
// 被弾ののけぞりが、あとから来た攻撃入力で上書きされないようにするため
template<class T>
class StateManager {
public:
    using StatePtr = std::unique_ptr<ICharacterState<T>>;

private:
    StatePtr _current;
    StatePtr _next;

    // 死亡のように必ず通したい切り替えが入っているか
    bool _isNextForced = false;

    // 抜けた状態は次のフレームの頭まで生かしておく
    std::vector<StatePtr> _retired;

public:
    void Start(T& owner, StatePtr first) {
        _current = std::move(first);
        if (_current) _current->Enter(owner);
    }

    // from が今の状態と違えば、古い判断から来た要求なので捨てる
    bool Transition(const ICharacterState<T>* from, StatePtr next) {
        if (!next) return false;
        if (from != _current.get()) return false;
        if (_next) return false;

        _next = std::move(next);
        return true;
    }

    // 先に入っている要求があっても上書きする
    void ForceTransition(StatePtr next) {
        if (!next) return;
        if (_isNextForced) return;

        _next = std::move(next);
        _isNextForced = true;
    }

    void Update(T& owner, const InputInfo& input, float deltaTime) {
        _retired.clear();

        // 前のフレームの後半に外から来た要求を先に通す
        Apply(owner);

        if (_current) _current->Execute(owner, input, deltaTime);

        Apply(owner);
    }

    ICharacterState<T>* GetCurrent() const { return _current.get(); }

    const char* GetCurrentName() const {
        return _current ? _current->GetName() : "-";
    }

    template<class S>
    bool IsIn() const {
        return dynamic_cast<const S*>(_current.get()) != nullptr;
    }

private:
    void Apply(T& owner) {
        if (!_next) return;

        if (_current) {
            _current->Exit(owner);
            _retired.push_back(std::move(_current));
        }

        _current = std::move(_next);
        _isNextForced = false;
        _current->Enter(owner);
    }
};
