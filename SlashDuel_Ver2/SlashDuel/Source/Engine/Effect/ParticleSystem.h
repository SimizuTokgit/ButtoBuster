#pragma once
#include "Component.h"
#include "Particle.h"
#include "DxLib.h"
#include <vector>

/// <summary>
/// パーティクルシステムコンポーネント
/// UnityのParticleSystemに相当
/// パーティクルの放出・物理・描画を統合管理する
/// </summary>
class ParticleSystem : public Component {
public:
    // 粒が残ったまま消されたときに、マネージャーが消えたものを回さないように外す
    ~ParticleSystem() override;

    // === Emission ===

    /// <summary>最大パーティクル数</summary>
    int maxParticles = 256;

    /// <summary>放出形状</summary>
    EmissionShape emissionShape = EmissionShape::Point;

    /// <summary>球体放出時の半径</summary>
    float emissionRadius = 0.0f;

    // 飛ばす向きと広がり 広がりが 180 度なら全方向
    // 斬った向きに火花を飛ばすときに絞る
    VECTOR emitDirection = VGet(0, 1, 0);
    float spreadDegree = 180.0f;

    /// <summary>秒あたりの放出数（0 = バーストのみ）</summary>
    float emissionRate = 0.0f;

    // === Lifetime ===

    /// <summary>初期ライフタイム（最小値、秒）</summary>
    float startLifetimeMin = 0.5f;

    /// <summary>初期ライフタイム（最大値、秒）</summary>
    float startLifetimeMax = 1.0f;

    // === Speed ===

    /// <summary>初期速度（最小値）</summary>
    float startSpeedMin = 200.0f;

    /// <summary>初期速度（最大値）</summary>
    float startSpeedMax = 750.0f;

    // === Size ===

    /// <summary>初期サイズ（最小値）</summary>
    float startSizeMin = 4.0f;

    /// <summary>初期サイズ（最大値）</summary>
    float startSizeMax = 16.0f;

    // === Color ===

    /// <summary>描画色</summary>
    COLOR_U8 color = GetColorU8(255, 255, 255, 255);

    // === Physics ===

    /// <summary>重力</summary>
    VECTOR gravity = VGet(0, 0, 0);

    /// <summary>減速量（速度単位/秒）</summary>
    float drag = 0.0f;

    // === Rendering ===

    /// <summary>パーティクル画像ハンドル</summary>
    int graphHandle = -1;

    /// <summary>ブレンドモード（DX_BLENDMODE_ADD等）</summary>
    int blendMode = DX_BLENDMODE_ADD;

    /// <summary>二重パス描画（SUB + ADD_X4、旧Damageエフェクト互換）</summary>
    bool useSubPass = false;

    // 0 より大きければ、進む向きに尾を引いた板で描く 速さにこの秒数を掛けた長さになる
    // 火花を丸ではなく線に見せるため
    float stretch = 0.0f;

    /// <summary>
    /// ライフタイムの何割地点からアルファフェードを開始するか（0.0〜1.0）
    /// 0.0 = 最初からフェード（デフォルト）、0.7 = 70%経過後にフェード開始
    /// 旧エフェクトのように一定時間フルアルファ維持→急速フェードを再現する
    /// </summary>
    float alphaFadeRatio = 0.0f;

    // === Control ===

    /// <summary>ループ再生</summary>
    bool looping = false;

    /// <summary>再生時間（秒、looping=true時のエミッション期間）</summary>
    float duration = 1.0f;

    // === Public Methods ===

    /// <summary>パーティクルの放出を開始する</summary>
    void Play();

    /// <summary>パーティクルの放出を停止する（既存パーティクルは残る）</summary>
    void Stop();

    /// <summary>放出中かどうか</summary>
    bool IsPlaying() const;

    /// <summary>生存しているパーティクルがあるか（放出中含む）</summary>
    bool IsAlive() const;

    /// <summary>指定数のパーティクルを即座に放出する</summary>
    void Emit(int count);

    // === Internal（ParticleManagerから呼ばれる） ===

    void Update(float deltaTime);
    void Render();

private:
    std::vector<Particle> _particles;
    bool _playing = false;
    float _elapsedTime = 0.0f;
    float _emissionAccumulator = 0.0f;

    void EmitSingle();
    static float RandomFloat(float minVal, float maxVal);
    static VECTOR RandomDirection();
    VECTOR RandomEmitDirection() const;
    void RenderStretched();

    // 尾を引く板の頂点 毎フレーム作り直すので持っておいて使い回す
    std::vector<VERTEX3D> _vertices;
    VECTOR RandomPointInSphere(float radius);
};
