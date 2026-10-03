#pragma once
#include "Object.h"
#include <vector>

class Transform;
class GameObject;

class Component : public Object{
public:
	Transform* transform = nullptr;
    GameObject* gameObject = nullptr;

public:
	Component() = default;

    virtual ~Component() = default;

    // ===== GetComponent ショートカット =====
    // gameObject の同名メソッドへ委譲する

    template<typename T>
    T* GetComponent();

    template<typename T>
    T* GetComponentInChildren();

    template<typename T>
    std::vector<T*> GetComponentsInChildren();

    template<typename T>
    T* GetComponentInParent();

    template<typename T>
    std::vector<T*> GetComponentsInParent();
};
