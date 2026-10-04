#pragma once
#include <algorithm>
#include <vector>

template<class Event>
class Observer;

// 起きたことを、登録された Observer へまとめて知らせる
// 知らせる側は受け取る側の型を知らないので、音やエフェクトを足しても知らせる側を書き換えずに済む
//
// Subject と Observer はお互いを覚えておき、先に消えたほうが相手から自分を外す
// どちらが先に消えても、消えた相手のポインタが残らない
template<class Event>
class Subject {
private:
    friend class Observer<Event>;

    std::vector<Observer<Event>*> _observers;

public:
    Subject() = default;

    // コピーすると、Observer が覚えている相手と食い違うのでさせない
    Subject(const Subject&) = delete;
    Subject& operator=(const Subject&) = delete;

    ~Subject() {
        for (auto* observer : _observers) observer->ForgetSubject(this);
    }

    void AddObserver(Observer<Event>* observer) {
        if (!observer || Contains(observer)) return;
        _observers.push_back(observer);
        observer->RememberSubject(this);
    }

    void RemoveObserver(Observer<Event>* observer) {
        if (!Contains(observer)) return;
        Erase(observer);
        observer->ForgetSubject(this);
    }

    void Notify(const Event& event) {
        // 知らせている途中で一覧が変わっても回し続けられるよう、控えてから回す
        // 控えたあとに外れた Observer には知らせない
        std::vector<Observer<Event>*> targets = _observers;
        for (auto* observer : targets) {
            if (Contains(observer)) observer->OnNotify(event);
        }
    }

private:
    bool Contains(const Observer<Event>* observer) const {
        return std::find(_observers.begin(), _observers.end(), observer) != _observers.end();
    }

    void Erase(const Observer<Event>* observer) {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
    }
};

// Subject から知らせを受け取る側
// 受け取ったときの処理を OnNotify に書く
template<class Event>
class Observer {
private:
    friend class Subject<Event>;

    std::vector<Subject<Event>*> _subjects;

public:
    Observer() = default;
    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;

    virtual ~Observer() {
        for (auto* subject : _subjects) subject->Erase(this);
    }

    virtual void OnNotify(const Event& event) = 0;

private:
    void RememberSubject(Subject<Event>* subject) {
        _subjects.push_back(subject);
    }

    void ForgetSubject(const Subject<Event>* subject) {
        _subjects.erase(std::remove(_subjects.begin(), _subjects.end(), subject), _subjects.end());
    }
};
