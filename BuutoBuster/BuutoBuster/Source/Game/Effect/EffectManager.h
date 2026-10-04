#pragma once
#include "MonoBehaviour.h"
#include "ShapeEffectRenderer.h"
#include "DxLib.h"

class ParticleSystem;
class CameraFollow;
class ScreenFlash;

// 手応えの演出をまとめて出す
// 火花 煙 斬撃の弧 衝撃波 画面の光 ヒットストップ スロー 画面の揺れ カメラの寄り
//
// シーンに1つだけ置き、どこからでも Get() で呼べるようにする
// シーンをまたいで残る static にしないのは、前のシーンの GameObject を掴んだまま残るから
class EffectManager : public MonoBehaviour {
public:
    using ArcDesc = ShapeEffectRenderer::ArcDesc;

private:
    static inline EffectManager* _instance = nullptr;

    // 完全に止めると入力の手触りまで消えるので、ほんの少しだけ動かしておく
    static constexpr float HIT_STOP_TIME_SCALE = 0.05f;

    int _damageGraph = -1;
    int _deadGraph = -1;

    // 種類ごとに1つずつ持ち、出す場所を動かして使い回す
    // 粒は放った時点の位置で動くので、あとから場所を動かしても前の粒は崩れない
    ParticleSystem* _hitSpark = nullptr;
    ParticleSystem* _hitStreak = nullptr;
    ParticleSystem* _hitFlash = nullptr;
    ParticleSystem* _guardSpark = nullptr;
    ParticleSystem* _killBurst = nullptr;
    ParticleSystem* _killEmber = nullptr;
    ParticleSystem* _deathSmoke = nullptr;
    ParticleSystem* _spawnSmoke = nullptr;
    ParticleSystem* _healLight = nullptr;
    ParticleSystem* _dust = nullptr;
    ParticleSystem* _warningGlint = nullptr;
    ParticleSystem* _heavyGlint = nullptr;
    ParticleSystem* _shockDebris = nullptr;

    ShapeEffectRenderer* _shapes = nullptr;
    ScreenFlash* _screenFlash = nullptr;
    CameraFollow* _camera = nullptr;

    float _hitStopTimer = 0.0f;
    float _slowTimer = 0.0f;
    float _slowScale = 1.0f;
    float _baseTimeScale = 1.0f;

public:
    static EffectManager* Get() { return _instance; }

    ~EffectManager() override;

    // カメラを作ったあとで呼ぶ
    void Initialize(CameraFollow* camera);

    void Update(float deltaTime) override;

    // ----- 粒 -----

    // direction は押された向き 火花はその向きへ飛ぶ 0 なら全方向
    void PlayHit(VECTOR position, VECTOR direction);
    void PlayGuard(VECTOR position, VECTOR direction);

    // 倒した瞬間の弾ける光 倒れきったあとの煙は PlayDeath
    void PlayKill(VECTOR position, VECTOR direction);
    void PlayDeath(VECTOR position);
    // 地面の位置を渡す 煙は少し上に、輪は地面に出す
    void PlaySpawn(VECTOR groundPosition);
    void PlayHeal(VECTOR position);

    // 足元の土煙 着地や回避の踏み切り
    void PlayDust(VECTOR groundPosition, int count);

    // 敵が振りかぶった合図 重い技は赤く大きく光らせる
    void PlayWarning(VECTOR position, bool isHeavy);

    // ----- 形 -----

    void PlaySlashArc(const ArcDesc& desc);

    // 足元から広がる輪と、跳ね上がる破片
    void PlayShockwave(VECTOR groundPosition, float radius, COLOR_U8 color);

    // 範囲攻撃が来る場所を先に見せる 輪が広がりきった瞬間に当たる
    void PlayAreaWarning(VECTOR groundPosition, float radius, float seconds);

    // ----- 画面 -----

    void FlashScreen(unsigned int color, float alpha, float seconds);
    void ZoomPunch(float degrees, float seconds);
    void HitStop(float seconds);
    void Shake(float power, float seconds);

    // 実時間で seconds のあいだ、時間の流れを scale 倍にする
    void SlowMotion(float scale, float seconds);

    // フェーズの最後の1体を倒したとき 止めて寄って光らせ、倒した実感を残す
    void PlayFinalBlow(VECTOR position);

    // デバッグのスロー再生用 ヒットストップが明けたらこの速さに戻る
    void SetBaseTimeScale(float scale);
    float GetBaseTimeScale() const { return _baseTimeScale; }

private:
    ParticleSystem* CreateSystem(const char* name, int graph);
    void Burst(ParticleSystem* system, VECTOR position, int count);
    void BurstToward(ParticleSystem* system, VECTOR position, VECTOR direction, float spreadDegree, int count);
    void ApplyTimeScale();
};
