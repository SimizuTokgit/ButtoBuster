#pragma once
#include "Collider.h"

/// <summary>
/// カプセルの向き
/// </summary>
enum class CapsuleDirection {
    XAxis,  // X軸方向（横向き）
    YAxis,  // Y軸方向（縦向き）- デフォルト
    ZAxis   // Z軸方向（奥行き方向）
};

/// <summary>
/// カプセル形状のコライダー
/// 主にキャラクターの当たり判定に使用
/// </summary>
class CapsuleCollider : public Collider {
public:
    /// <summary>カプセルの半径</summary>
    float radius = 0.5f;

    /// <summary>カプセルの全長（両端の半球を含む高さ）</summary>
    float height = 2.0f;

    /// <summary>カプセルの向き</summary>
    CapsuleDirection direction = CapsuleDirection::YAxis;

public:
    CapsuleCollider() = default;
    ~CapsuleCollider() override = default;

    /// <summary>
    /// カプセルを構成する2点をワールド座標で取得
    /// Transformの回転を適用し、カプセルの向きがオブジェクトの向きに追従する
    /// </summary>
    /// <param name="point1">カプセルの下端（または始点）</param>
    /// <param name="point2">カプセルの上端（または終点）</param>
    void GetCapsulePoints(VECTOR& point1, VECTOR& point2) const {
        VECTOR worldCenter = GetWorldCenter();
        
        // 円柱部分の半分の長さ
        float halfCylinder = GetCylinderHeight() / 2.0f;

        // カプセルの向きに応じてローカル方向のオフセットを計算
        VECTOR localDir = VGet(0, 0, 0);
        switch (direction) {
        case CapsuleDirection::XAxis:
            localDir = VGet(halfCylinder, 0, 0);
            break;
        case CapsuleDirection::YAxis:
            localDir = VGet(0, halfCylinder, 0);
            break;
        case CapsuleDirection::ZAxis:
            localDir = VGet(0, 0, halfCylinder);
            break;
        }

        // Transformのワールド回転を適用
        if (transform) {
            Quaternion worldRot = transform->rotation;
            localDir = worldRot * localDir;
        }

        point1 = VSub(worldCenter, localDir);
        point2 = VAdd(worldCenter, localDir);
    }

    /// <summary>ワールド座標での中心位置を取得</summary>
    VECTOR GetWorldCenter() const override {
        if (transform) {
            Quaternion worldRot = transform->rotation;
            VECTOR rotatedCenter = worldRot * center;
            return VAdd(transform->position, rotatedCenter);
        }
        return center;
    }

    /// <summary>
    /// カプセルの円柱部分の長さを取得（半球を除いた部分）
    /// </summary>
    float GetCylinderHeight() const {
        // 全長から両端の半球（radius * 2）を引いた長さ
        // 最小値は0（球になる場合）
        float cylinderHeight = height - radius * 2.0f;
        return (cylinderHeight > 0.0f) ? cylinderHeight : 0.0f;
    }

    /// <summary>
    /// デバッグ用のカプセル描画
    /// </summary>
    /// <param name="color">描画色（デフォルト: 緑）</param>
    /// <param name="filled">塗りつぶすかどうか</param>
    void DrawGizmo(unsigned int color = 0x00FF00, bool filled = false) const {
        VECTOR point1, point2;
        GetCapsulePoints(point1, point2);

        // Zバッファを使用して描画
        SetUseZBufferFlag(TRUE);
        SetWriteZBufferFlag(TRUE);
        SetUseLighting(FALSE);

        // カプセルを描画（DxLibの関数を使用）
        DrawCapsule3D(
            point1,
            point2,
            radius,
            8,  // 分割数
            color,
            color,
            filled ? TRUE : FALSE
        );

        // 描画設定を元に戻す
        SetUseLighting(TRUE);
        SetWriteZBufferFlag(FALSE);
        SetUseZBufferFlag(FALSE);
    }
};
