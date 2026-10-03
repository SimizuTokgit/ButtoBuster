#pragma once
#include "Component.h"
#include "Quaternion.h"
#include "DxLib.h"
#include <vector>
#include <cmath>

/// <summary>
/// Transform コンポーネント
/// UnityのTransform相当
/// Local（親基準）とWorld（ワールド基準）の座標系を持つ
/// 回転はQuaternionで管理する（Unityと同じ）
/// 親子関係の本体（Unityと同じ設計）
/// </summary>
class Transform : public Component {
public:
    /// <summary>度数法→ラジアン変換</summary>
    static float Deg2Rad(float deg) { return deg * DX_PI_F / 180.0f; }
    /// <summary>ラジアン→度数法変換</summary>
    static float Rad2Deg(float rad) { return rad * 180.0f / DX_PI_F; }

    // ===== ローカル座標系（親オブジェクト基準） =====

    /// <summary>ローカル座標（親がいない場合はワールド座標と同じ）</summary>
    VECTOR localPosition = VGet(0, 0, 0);

    /// <summary>ローカル回転（Quaternion）</summary>
    Quaternion localRotation = Quaternion::Identity();

    /// <summary>ローカルスケール</summary>
    VECTOR localScale = VGet(1, 1, 1);

    /// <summary>親Transform</summary>
    Transform* parent = nullptr;

    /// <summary>
    /// trueの場合、ワールド座標計算で親の変換を無視する
    /// BoneFollower等、ワールド値をlocalに直接書き込むコンポーネント用
    /// </summary>
    bool ignoreParentTransform = false;

private:
    /// <summary>子Transformのリスト</summary>
    std::vector<Transform*> _children;

public:
    Transform() = default;

    /// <summary>
    /// 破棄時に親子のリンクを解除する
    ///
    /// これが無いと、GameObjectを実行中に破棄したときに
    /// 親の_childrenへ解放済みポインタが残り、次の走査でクラッシュする
    /// </summary>
    virtual ~Transform() {
        // 親の子リストから自分を外す
        if (parent) {
            parent->RemoveChild(this);
            parent = nullptr;
        }

        // 子から自分への参照を切る
        // （子GameObjectはこのTransformより先に破棄されるため通常は空になっている）
        for (auto* child : _children) {
            if (child) child->parent = nullptr;
        }
        _children.clear();
    }

    // ===== プロパティ（Unityスタイル） =====

    /// <summary>位置（localPositionと同じ）</summary>
    __declspec(property(get = GetPosition, put = SetPosition)) VECTOR localPosition;

    /// <summary>ローカル回転（Quaternion）</summary>
    __declspec(property(get = GetLocalRotation, put = SetLocalRotation)) Quaternion localRotation;

    /// <summary>ローカル回転（オイラー角・度数法）</summary>
    __declspec(property(get = GetLocalEulerAngles, put = SetLocalEulerAngles)) VECTOR localEulerAngles;

    /// <summary>スケール（自身のスケール値をそのまま取得・設定）</summary>
    __declspec(property(get = GetScale, put = SetScale)) VECTOR localScale;

    /// <summary>ワールド座標</summary>
    __declspec(property(get = GetWorldPosition, put = SetWorldPosition)) VECTOR position;

    /// <summary>ワールド回転（Quaternion）</summary>
    __declspec(property(get = GetWorldRotation, put = SetWorldRotation)) Quaternion rotation;

    /// <summary>ワールド回転（オイラー角・度数法）</summary>
    __declspec(property(get = GetWorldEulerAngles, put = SetWorldEulerAngles)) VECTOR eulerAngles;

    /// <summary>ワールドスケール（読み取り専用）</summary>
    __declspec(property(get = GetWorldScale)) VECTOR worldScale;

    /// <summary>前方ベクトル（読み取り専用）</summary>
    __declspec(property(get = GetForward)) VECTOR forward;

    /// <summary>右方向ベクトル（読み取り専用）</summary>
    __declspec(property(get = GetRight)) VECTOR right;

    // ===== プロパティ実装 =====

    VECTOR GetPosition() const { return localPosition; }
    void SetPosition(const VECTOR& value) { localPosition = value; }

    Quaternion GetLocalRotation() const { return localRotation; }
    void SetLocalRotation(const Quaternion& value) { localRotation = value; }

    VECTOR GetLocalEulerAngles() const { return localRotation.ToEuler(); }
    void SetLocalEulerAngles(const VECTOR& value) { localRotation = Quaternion::Euler(value); }

    VECTOR GetScale() const { return localScale; }
    void SetScale(const VECTOR& value) { localScale = value; }

    VECTOR GetWorldScale() const {
        if (parent && !ignoreParentTransform) {
            VECTOR parentWorld = parent->worldScale;
            return VGet(
                parentWorld.x * localScale.x,
                parentWorld.y * localScale.y,
                parentWorld.z * localScale.z
            );
        }
        return localScale;
    }

    VECTOR GetWorldPosition() const {
        if (parent && !ignoreParentTransform) {
            VECTOR parentWorldPos = parent->position;
            Quaternion parentWorldRot = parent->rotation;
            VECTOR parentWorldScale = parent->worldScale;

            VECTOR scaledLocal = VGet(
                localPosition.x * parentWorldScale.x,
                localPosition.y * parentWorldScale.y,
                localPosition.z * parentWorldScale.z
            );

            VECTOR rotatedLocal = parentWorldRot * scaledLocal;
            return VAdd(parentWorldPos, rotatedLocal);
        }
        return localPosition;
    }

    void SetWorldPosition(const VECTOR& worldPos) {
        if (parent && !ignoreParentTransform) {
            VECTOR parentWorldPos = parent->position;
            Quaternion parentWorldRot = parent->rotation;
            VECTOR parentWorldScale = parent->worldScale;

            VECTOR relativePos = VSub(worldPos, parentWorldPos);
            VECTOR unrotated = parentWorldRot.Inverse() * relativePos;

            localPosition = VGet(
                parentWorldScale.x != 0 ? unrotated.x / parentWorldScale.x : 0,
                parentWorldScale.y != 0 ? unrotated.y / parentWorldScale.y : 0,
                parentWorldScale.z != 0 ? unrotated.z / parentWorldScale.z : 0
            );
        }
        else {
            localPosition = worldPos;
        }
    }

    Quaternion GetWorldRotation() const {
        if (parent && !ignoreParentTransform) {
            return parent->rotation * localRotation;
        }
        return localRotation;
    }

    void SetWorldRotation(const Quaternion& worldRot) {
        if (parent && !ignoreParentTransform) {
            localRotation = parent->rotation.Inverse() * worldRot;
        }
        else {
            localRotation = worldRot;
        }
    }

    VECTOR GetWorldEulerAngles() const { return rotation.ToEuler(); }
    void SetWorldEulerAngles(const VECTOR& value) { rotation = Quaternion::Euler(value); }

    VECTOR GetForward() const {
        return rotation * VGet(0, 0, 1);
    }

    VECTOR GetRight() const {
        return rotation * VGet(1, 0, 0);
    }

    // ===== 親子関係 =====

    /// <summary>親Transformを設定（Unityと同じくTransformが親子関係の本体）</summary>
    void SetParent(Transform* newParent) {
        // 現在の親から自分を除去
        if (parent) {
            parent->RemoveChild(this);
        }
        parent = newParent;
        // 新しい親に自分を追加
        if (parent) {
            parent->AddChild(this);
        }
    }

    /// <summary>子Transformの数を取得</summary>
    size_t GetChildCount() const { return _children.size(); }

    /// <summary>インデックスで子Transformを取得</summary>
    Transform* GetChild(size_t index) const {
        if (index < _children.size()) {
            return _children[index];
        }
        return nullptr;
    }

private:
    /// <summary>子Transformを追加（内部用）</summary>
    void AddChild(Transform* child) {
        if (child && child != this) {
            for (auto* c : _children) {
                if (c == child) return;
            }
            _children.push_back(child);
        }
    }

    /// <summary>子Transformを除去（内部用）</summary>
    void RemoveChild(Transform* child) {
        for (auto it = _children.begin(); it != _children.end(); ++it) {
            if (*it == child) {
                _children.erase(it);
                return;
            }
        }
    }
};

