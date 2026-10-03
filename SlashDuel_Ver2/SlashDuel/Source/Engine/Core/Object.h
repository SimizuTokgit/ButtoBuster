#pragma once
#include <vector>

// 前方宣言
class Scene;

/// <summary>
/// UnityのObject相当
/// 全てのゲームオブジェクトとコンポーネントの基底クラス
/// </summary>
class Object {
public:
    Object() = default;

    virtual ~Object() = default;

    // ===== 静的Find機能 =====

    /// <summary>
    /// 指定した型のコンポーネントを持つ最初のオブジェクトからそのコンポーネントを取得
    /// UnityのFindFirstObjectByType相当
    /// </summary>
    template<typename T>
    static T* FindFirstObjectByType();

    /// <summary>
    /// 指定した型のコンポーネントを全て取得
    /// UnityのFindObjectsByType相当
    /// </summary>
    template<typename T>
    static std::vector<T*> FindObjectsByType();
};
