#pragma once
#include "DxLib.h"
#include <cmath>

/// <summary>
/// クォータニオン構造体
/// UnityのQuaternion相当
/// 3D回転を正しく合成するために使用する
/// </summary>
struct Quaternion {
    float x, y, z, w;

    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    /// <summary>単位クォータニオン（回転なし）</summary>
    static Quaternion Identity() {
        return Quaternion(0, 0, 0, 1);
    }

    /// <summary>
    /// ディグリー角のXYZオイラー角からクォータニオンを生成
    /// 回転適用順序: Y → X → Z（Unityと同じ）
    /// </summary>
    static Quaternion Euler(float degX, float degY, float degZ) {
        float rx = degX * DX_PI_F / 180.0f * 0.5f;
        float ry = degY * DX_PI_F / 180.0f * 0.5f;
        float rz = degZ * DX_PI_F / 180.0f * 0.5f;

        float cx = cosf(rx), sx = sinf(rx);
        float cy = cosf(ry), sy = sinf(ry);
        float cz = cosf(rz), sz = sinf(rz);

        // Y → X → Z の合成順序
        return Quaternion(
            cy * sx * cz + sy * cx * sz,
            sy * cx * cz - cy * sx * sz,
            cy * cx * sz - sy * sx * cz,
            cy * cx * cz + sy * sx * sz
        );
    }

    /// <summary>
    /// VECTORのディグリー角オイラーからクォータニオンを生成
    /// </summary>
    static Quaternion Euler(const VECTOR& eulerDeg) {
        return Euler(eulerDeg.x, eulerDeg.y, eulerDeg.z);
    }

    /// <summary>
    /// 回転行列からクォータニオンを生成
    /// スケールが除去された純粋な回転行列を前提とする
    /// </summary>
    static Quaternion FromMatrix(const MATRIX& m) {
        float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];
        Quaternion q;

        if (trace > 0.0f) {
            float s = sqrtf(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (m.m[1][2] - m.m[2][1]) / s;
            q.y = (m.m[2][0] - m.m[0][2]) / s;
            q.z = (m.m[0][1] - m.m[1][0]) / s;
        }
        else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
            float s = sqrtf(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f;
            q.w = (m.m[1][2] - m.m[2][1]) / s;
            q.x = 0.25f * s;
            q.y = (m.m[0][1] + m.m[1][0]) / s;
            q.z = (m.m[2][0] + m.m[0][2]) / s;
        }
        else if (m.m[1][1] > m.m[2][2]) {
            float s = sqrtf(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f;
            q.w = (m.m[2][0] - m.m[0][2]) / s;
            q.x = (m.m[0][1] + m.m[1][0]) / s;
            q.y = 0.25f * s;
            q.z = (m.m[1][2] + m.m[2][1]) / s;
        }
        else {
            float s = sqrtf(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f;
            q.w = (m.m[0][1] - m.m[1][0]) / s;
            q.x = (m.m[2][0] + m.m[0][2]) / s;
            q.y = (m.m[1][2] + m.m[2][1]) / s;
            q.z = 0.25f * s;
        }

        return q;
    }

    /// <summary>
    /// クォータニオンをディグリー角のXYZオイラー角に変換
    /// </summary>
    VECTOR ToEuler() const {
        // Y → X → Z の順序に対応するオイラー角抽出
        float sinX = 2.0f * (w * x - y * z);
        if (sinX > 1.0f) sinX = 1.0f;
        if (sinX < -1.0f) sinX = -1.0f;

        float ex, ey, ez;

        if (fabsf(sinX) > 0.9999f) {
            // ジンバルロック付近
            ex = 90.0f * sinX;
            ey = 180.0f / DX_PI_F * atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (y * y + x * x));
            ez = 0.0f;
        }
        else {
            ex = 180.0f / DX_PI_F * asinf(sinX);
            ey = 180.0f / DX_PI_F * atan2f(2.0f * (w * y + x * z), 1.0f - 2.0f * (x * x + y * y));
            ez = 180.0f / DX_PI_F * atan2f(2.0f * (w * z + x * y), 1.0f - 2.0f * (x * x + z * z));
        }

        return VGet(ex, ey, ez);
    }

    /// <summary>
    /// クォータニオンから回転行列を生成
    /// </summary>
    MATRIX ToMatrix() const {
        MATRIX m = MGetIdent();

        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        m.m[0][0] = 1.0f - 2.0f * (yy + zz);
        m.m[0][1] = 2.0f * (xy + wz);
        m.m[0][2] = 2.0f * (xz - wy);

        m.m[1][0] = 2.0f * (xy - wz);
        m.m[1][1] = 1.0f - 2.0f * (xx + zz);
        m.m[1][2] = 2.0f * (yz + wx);

        m.m[2][0] = 2.0f * (xz + wy);
        m.m[2][1] = 2.0f * (yz - wx);
        m.m[2][2] = 1.0f - 2.0f * (xx + yy);

        return m;
    }

    /// <summary>
    /// ベクトルをこのクォータニオンで回転する
    /// q * v * q^-1 と等価
    /// </summary>
    VECTOR operator*(const VECTOR& v) const {
        // t = 2 * cross(q.xyz, v)
        float tx = 2.0f * (y * v.z - z * v.y);
        float ty = 2.0f * (z * v.x - x * v.z);
        float tz = 2.0f * (x * v.y - y * v.x);

        // result = v + w * t + cross(q.xyz, t)
        return VGet(
            v.x + w * tx + (y * tz - z * ty),
            v.y + w * ty + (z * tx - x * tz),
            v.z + w * tz + (x * ty - y * tx)
        );
    }

    /// <summary>
    /// クォータニオン同士の乗算（回転の合成）
    /// result = this * q （thisの回転の後にqの回転を適用...ではなく
    /// 親の回転thisの後に子の回転qを適用する場合: parent * child）
    /// </summary>
    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        );
    }

    /// <summary>
    /// 逆クォータニオン（共役 / ノルムの2乗）
    /// 正規化済みクォータニオンの場合は共役と等価
    /// </summary>
    Quaternion Inverse() const {
        float n = x * x + y * y + z * z + w * w;
        if (n < 1e-10f) return Identity();
        float invN = 1.0f / n;
        return Quaternion(-x * invN, -y * invN, -z * invN, w * invN);
    }

    // ===== 比較演算子 =====

    bool operator==(const Quaternion& q) const {
        return x == q.x && y == q.y && z == q.z && w == q.w;
    }

    bool operator!=(const Quaternion& q) const {
        return !(*this == q);
    }

    // ===== ユーティリティ =====

    /// <summary>
    /// クォータニオンの大きさ（ノルム）を取得
    /// </summary>
    float Magnitude() const {
        return sqrtf(x * x + y * y + z * z + w * w);
    }

    /// <summary>
    /// 正規化したクォータニオンを返す
    /// </summary>
    Quaternion Normalized() const {
        float mag = Magnitude();
        if (mag < 1e-10f) return Identity();
        float inv = 1.0f / mag;
        return Quaternion(x * inv, y * inv, z * inv, w * inv);
    }

    /// <summary>
    /// 軸と角度（度）からクォータニオンを生成
    /// </summary>
    static Quaternion AngleAxis(float angleDeg, VECTOR axis) {
        float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
        if (len < 1e-10f) return Identity();
        float inv = 1.0f / len;
        axis.x *= inv; axis.y *= inv; axis.z *= inv;

        float halfRad = angleDeg * DX_PI_F / 180.0f * 0.5f;
        float s = sinf(halfRad);
        return Quaternion(axis.x * s, axis.y * s, axis.z * s, cosf(halfRad));
    }

    /// <summary>
    /// 前方ベクトルと上方ベクトルから回転を生成（UnityのQuaternion.LookRotation相当）
    /// </summary>
    static Quaternion LookRotation(VECTOR forward, VECTOR up = VGet(0, 1, 0)) {
        // forward を正規化
        float fLen = sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
        if (fLen < 1e-10f) return Identity();
        float fInv = 1.0f / fLen;
        forward.x *= fInv; forward.y *= fInv; forward.z *= fInv;

        // right = cross(up, forward)
        VECTOR right = VGet(
            up.y * forward.z - up.z * forward.y,
            up.z * forward.x - up.x * forward.z,
            up.x * forward.y - up.y * forward.x
        );
        float rLen = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
        if (rLen < 1e-10f) return Identity();
        float rInv = 1.0f / rLen;
        right.x *= rInv; right.y *= rInv; right.z *= rInv;

        // up = cross(forward, right)  再計算で直交化
        up = VGet(
            forward.y * right.z - forward.z * right.y,
            forward.z * right.x - forward.x * right.z,
            forward.x * right.y - forward.y * right.x
        );

        // 回転行列 [right, up, forward] からクォータニオンを生成
        MATRIX m = MGetIdent();
        m.m[0][0] = right.x;   m.m[0][1] = up.x;   m.m[0][2] = forward.x;
        m.m[1][0] = right.y;   m.m[1][1] = up.y;   m.m[1][2] = forward.y;
        m.m[2][0] = right.z;   m.m[2][1] = up.z;   m.m[2][2] = forward.z;
        return FromMatrix(m);
    }

    /// <summary>
    /// 球面線形補間（Slerp）
    /// </summary>
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) {
        float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

        // 最短経路を選択
        Quaternion target = b;
        if (dot < 0.0f) {
            target = Quaternion(-b.x, -b.y, -b.z, -b.w);
            dot = -dot;
        }

        // dotが1に近い場合はLerpで近似
        if (dot > 0.9995f) {
            return Quaternion(
                a.x + t * (target.x - a.x),
                a.y + t * (target.y - a.y),
                a.z + t * (target.z - a.z),
                a.w + t * (target.w - a.w)
            ).Normalized();
        }

        float theta = acosf(dot);
        float sinTheta = sinf(theta);
        float wa = sinf((1.0f - t) * theta) / sinTheta;
        float wb = sinf(t * theta) / sinTheta;

        return Quaternion(
            wa * a.x + wb * target.x,
            wa * a.y + wb * target.y,
            wa * a.z + wb * target.z,
            wa * a.w + wb * target.w
        );
    }
};
