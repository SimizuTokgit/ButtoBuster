#pragma once
#include <memory>
#include <utility>
#include <vector>

// 節は毎フレーム Tick を呼ばれ、3 つのどれかを返す
enum class BehaviorStatus {
    Success,    // できた / 条件に合う
    Failure,    // できなかった / 条件に合わない
    Running,    // まだ途中 次のフレームも続ける
};

// 節の元 T は木を持つ側
template<typename T>
class BehaviorNode {
public:
    virtual ~BehaviorNode() = default;
    virtual BehaviorStatus Tick(T& owner, float deltaTime) = 0;
};

template<typename T>
using BehaviorNodePtr = std::unique_ptr<BehaviorNode<T>>;

// 子を上から試してFailure でなかった最初の子で止まる できるものを 1 つ選ぶ
template<typename T>
class BehaviorSelector : public BehaviorNode<T> {
private:
    std::vector<BehaviorNodePtr<T>> _children;

public:
    void Add(BehaviorNodePtr<T> child) { _children.push_back(std::move(child)); }

    BehaviorStatus Tick(T& owner, float deltaTime) override {
        for (auto& child : _children) {
            BehaviorStatus status = child->Tick(owner, deltaTime);
            if (status != BehaviorStatus::Failure) return status;
        }
        return BehaviorStatus::Failure;
    }
};

// 子を上から進め、Success でなかった子で止まる条件がそろったらやる
template<typename T>
class BehaviorSequence : public BehaviorNode<T> {
private:
    std::vector<BehaviorNodePtr<T>> _children;

public:
    void Add(BehaviorNodePtr<T> child) { _children.push_back(std::move(child)); }

    BehaviorStatus Tick(T& owner, float deltaTime) override {
        for (auto& child : _children) {
            BehaviorStatus status = child->Tick(owner, deltaTime);
            if (status != BehaviorStatus::Success) return status;
        }
        return BehaviorStatus::Success;
    }
};

// 持ち主の const なメンバー関数を呼ぶ
template<typename T>
class BehaviorCondition : public BehaviorNode<T> {
private:
    bool (T::* _check)() const;

public:
    explicit BehaviorCondition(bool (T::* check)() const) : _check(check) {}

    BehaviorStatus Tick(T& owner, float deltaTime) override {
        return (owner.*_check)() ? BehaviorStatus::Success : BehaviorStatus::Failure;
    }
};

// 持ち主のメンバー関数を呼び、その結果を返す
template<typename T>
class BehaviorAction : public BehaviorNode<T> {
private:
    BehaviorStatus(T::* _run)(float);

public:
    explicit BehaviorAction(BehaviorStatus(T::* run)(float)) : _run(run) {}

    BehaviorStatus Tick(T& owner, float deltaTime) override {
        return (owner.*_run)(deltaTime);
    }
};