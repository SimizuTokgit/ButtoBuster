#pragma once
#include "MonoBehaviour.h"
#include "ShapeEffectRenderer.h"
#include "SpriteEffectRenderer.h"
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
    using SpriteDesc = SpriteEffectRenderer::SpriteDesc;

private:
    static inline EffectManager* _instance = nullptr;

    // 完全に止めると入力の手触りまで消えるので、ほんの少しだけ動かしておく
    static constexpr float HIT_STOP_TIME_SCALE = 0.05f;

    int _damageGraph = -1;
    int _deadGraph = -1;

    // 板に貼って出す画像 必殺技の雷と、壁が割れたときのヒビ
    int _lightningGraph = -1;
    int _wallCrackGraph = -1;

    // 種類ごとに1つずつ持ち、出す場所を動かして使い回す
    // 粒は放った時点の位置で動くので、あとから場所を動かしても前の粒は崩れない
    ParticleSystem* _hitSpark = nullptr;
    ParticleSystem* _hitStreak = nullptr;
    ParticleSystem* _hitFlash = nullptr;
    ParticleSystem* _guardSpark = nullptr;
    ParticleSystem* _killBurst = nullptr;
    ParticleSystem* _killEmber = nullptr;
    ParticleSystem* _spawnSmoke = nullptr;
    ParticleSystem* _healLight = nullptr;
    ParticleSystem* _dust = nullptr;
    ParticleSystem* _warningGlint = nullptr;
    ParticleSystem* _heavyGlint = nullptr;
    ParticleSystem* _dodgeCueGlint = nullptr;
    ParticleSystem* _shockDebris = nullptr;
    ParticleSystem* _steam = nullptr;
    ParticleSystem* _wallShard = nullptr;

    ShapeEffectRenderer* _shapes = nullptr;
    SpriteEffectRenderer* _sprites = nullptr;
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

    // 地面の位置を渡す 煙は少し上に、輪は地面に出す
    void PlaySpawn(VECTOR groundPosition);
    void PlayHeal(VECTOR position);

    // 足元の土煙 着地や回避の踏み切り
    void PlayDust(VECTOR groundPosition, int count);

    // 敵が振りかぶった合図 重い技は赤く大きく光らせる
    void PlayWarning(VECTOR position, bool isHeavy);

    // ジャスト回避の合図 当たる直前に敵を白く光らせる 光ったら回避、で覚えられるように
    void PlayDodgeCue(VECTOR position);

    // 吹っ飛ばされ値が溜まった体から立ちのぼる湯気 溜まるほど多く呼ばれる
    void PlaySteam(VECTOR position, int count);

    // ----- 形 -----

    void PlaySlashArc(const ArcDesc& desc);

    // 足元から広がる輪と、跳ね上がる破片
    void PlayShockwave(VECTOR groundPosition, float radius, COLOR_U8 color);

    // 範囲攻撃が来る場所を先に見せる 輪が広がりきった瞬間に当たる
    void PlayAreaWarning(VECTOR groundPosition, float radius, float seconds);

    // ----- 画像の板 -----
    // 画像はここで読んだものを使い、場所 大きさ 長さ 濃さは desc で決める (desc.graph は入れなくてよい)

    // 必殺技の雷 (Data/Effect/Lightning.png) 画像の下端が雷の落ちた所になる
    void PlayLightning(SpriteDesc desc);

    // 壁のヒビ (Data/Effect/WallCrack.png)
    void PlayWallCrack(SpriteDesc desc);

    // ----- 画面 -----

    void FlashScreen(unsigned int color, float alpha, float seconds);
    void ZoomPunch(float degrees, float seconds);
    void HitStop(float seconds);
    void Shake(float power, float seconds);

    // 実時間で seconds のあいだ、時間の流れを scale 倍にする
    void SlowMotion(float scale, float seconds);

    // フェーズの最後の1体を倒したとき 止めて寄って光らせ、倒した実感を残す
    void PlayFinalBlow(VECTOR position);

    // 吹き飛んだ体が壁にぶつかって跳ね返ったとき 内側へ火花を散らし、ぶつかった所の光の幕を光らせる
    // normal は壁から内側への向き power は 0〜1 の強さで、速くぶつかるほど大きく止めて揺らす
    void PlayWallHit(VECTOR position, VECTOR normal, float power);

    // 壁が割れたときの粒だけ出す 割れた壁のかけらを外へ散らし、割れた所を光らせる
    // 止める 揺らす 画面を光らせるといった演出は、壁割りの知らせを受けた WallBreakEffectObserver が決める
    void PlayWallBreak(VECTOR position, VECTOR outward);

    // デバッグのスロー再生用 ヒットストップが明けたらこの速さに戻る
    void SetBaseTimeScale(float scale);
    float GetBaseTimeScale() const { return _baseTimeScale; }

private:
    ParticleSystem* CreateSystem(const char* name, int graph);
    void Burst(ParticleSystem* system, VECTOR position, int count);
    void BurstToward(ParticleSystem* system, VECTOR position, VECTOR direction, float spreadDegree, int count);
    void ApplyTimeScale();
};
