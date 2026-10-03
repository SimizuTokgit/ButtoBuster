#pragma once
#include "Component.h"

/// <summary>
/// 有効/無効を切り替えられるコンポーネントの基底クラス
/// UnityのBehaviour相当
/// </summary>
class Behaviour : public Component {
public:
    /// <summary>コンポーネントの有効/無効</summary>
    bool enabled = true;

public:
    Behaviour() = default;

    virtual ~Behaviour() = default;
};
