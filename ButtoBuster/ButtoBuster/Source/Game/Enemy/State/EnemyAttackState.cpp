#include "EnemyAttackState.h"
#include "Enemy.h"
#include "EnemyActions.h"
#include "EnemyIdleState.h"
#include "CombatSystem.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include <memory>

using std::make_unique;

namespace {
    // アニメの時間は 30fps のフレームで数えている
    constexpr float ANIMATION_FPS = 30.0f;

    // ----- 攻撃の合図 (頭の上のオレンジ 大振りは赤) -----
    // 当たり判定が出るこの秒数前に光らせて音を鳴らす 光った瞬間に回避すればジャスト回避になる
    // ジャスト回避は、押してから justDodgeWindow (PlayerData 0.18 秒) のうちに当たったときなので、それより短くする
    // 前は振りかぶった瞬間に光らせていたが、当たるまでが長くて回避の合図にならなかった
    constexpr float WARNING_LEAD_SECONDS = 0.12f;
    constexpr const char* WARNING_SOUND = "Player/equip";
    constexpr float WARNING_VOLUME = 0.6f;
}

EnemyAttackState::EnemyAttackState(const AttackData& data, float areaRadius)
    : _data(data)
    , _areaRadius(areaRadius) {
}

void EnemyAttackState::Enter(Enemy& enemy) {
    enemy.PlayAnimation(_data.animationName, _data.animationSpeed, true);
    enemy.FaceImmediately(EnemyActions::ToTarget(enemy));
    enemy.StopHorizontal();

    PlayAreaWarning(enemy);
}

void EnemyAttackState::Execute(Enemy& enemy, const InputInfo& input, float deltaTime) {
    float time = enemy.GetAnimationTime();

    // 振りかぶっている間は相手を追って向きを変える
    // 振り始める少し前に向きを固めて、避ける余地を残す
    constexpr float COMMIT_BEFORE_HIT = 4.0f;
    if (time < _data.hitStart - COMMIT_BEFORE_HIT) {
        enemy.FaceTowards(EnemyActions::ToTarget(enemy), enemy.GetData().turnSpeed * 0.6f, deltaTime);
    }

    bool isLunging = time >= _data.hitStart - COMMIT_BEFORE_HIT && time <= _data.hitEnd;
    if (isLunging) {
        enemy.SetHorizontalVelocity(enemy.GetForward(), _data.lunge);
    }
    else {
        enemy.StopHorizontal();
    }

    PlayWarning(enemy, time);
    PlaySwing(enemy, time);
    ApplyHit(enemy, time);

    if (enemy.IsAnimationFinished()) {
        enemy.GetStates().Transition(this, make_unique<EnemyIdleState>());
    }
}

void EnemyAttackState::Exit(Enemy& enemy) {
    enemy.StopHorizontal();

    // 途中で斬られて終わっても知らせる 攻撃の番を返してもらうため
    enemy.NotifyAttackFinished();
}

void EnemyAttackState::PlayAreaWarning(Enemy& enemy) {
    // 踏みつけは当たる範囲を先に地面へ出す 赤が満ちきったときに当たる
    // 輪は普段の時間で満ちるので、反撃の間に敵がゆっくりになっていると、満ちたあと少ししてから当たる
    if (_areaRadius <= 0.0f) return;

    auto* effects = EffectManager::Get();
    if (!effects) return;

    float seconds = _data.hitStart / (ANIMATION_FPS * _data.animationSpeed);
    effects->PlayAreaWarning(enemy.GetPosition(), _areaRadius, seconds);
}

void EnemyAttackState::PlayWarning(Enemy& enemy, float time) {
    // 光らせるのは、判定が出るこの数だけ前のフレーム (アニメの速さで変わる)
    float leadFrames = WARNING_LEAD_SECONDS * ANIMATION_FPS * _data.animationSpeed;

    bool isFirst = !_hasPlayedWarning && time >= _data.hitStart - leadFrames;
    if (isFirst) _hasPlayedWarning = true;

    // 二段斬りは、二段目の前にももう一度光らせる
    bool hasSecond = _data.hitStart2 >= 0.0f;
    bool isSecond = hasSecond && !_hasPlayedSecondWarning && time >= _data.hitStart2 - leadFrames;
    if (isSecond) _hasPlayedSecondWarning = true;

    if (!isFirst && !isSecond) return;

    // 頭の上を光らせる 囲まれていても、誰の攻撃が来るか分かるように 大振りと踏みつけは赤く大きく
    if (auto* effects = EffectManager::Get()) {
        bool isHeavy = _areaRadius > 0.0f || !_data.canGuard;
        VECTOR head = VAdd(enemy.GetCenter(), VGet(0.0f, enemy.bodyHeight * 0.5f, 0.0f));
        effects->PlayWarning(head, isHeavy);
    }
    SoundManager::Instance().PlaySE(WARNING_SOUND, WARNING_VOLUME);
}

void EnemyAttackState::PlaySwing(Enemy& enemy, float time) {
    // 判定が出る少し前に出すと、振りと弧が重なって見える
    constexpr float SWING_LEAD = 0.5f;

    if (!_hasPlayedArc && time >= _data.hitStart - SWING_LEAD) {
        _hasPlayedArc = true;
        PlayArc(enemy, _data.arcSwing);
    }

    // 二段斬りは返す刀なので逆から振る
    bool hasSecond = _data.hitStart2 >= 0.0f;
    if (hasSecond && !_hasPlayedSecondArc && time >= _data.hitStart2 - SWING_LEAD) {
        _hasPlayedSecondArc = true;
        PlayArc(enemy, -_data.arcSwing);
    }
}

void EnemyAttackState::PlayArc(Enemy& enemy, float swing) {
    if (!_data.hasArc || _areaRadius > 0.0f) return;

    auto* effects = EffectManager::Get();
    if (!effects) return;

    EffectManager::ArcDesc arc;
    arc.center = VAdd(enemy.GetPosition(), VGet(0.0f, enemy.bodyHeight * 0.5f, 0.0f));
    arc.forward = enemy.GetForward();
    arc.radius = _data.reach * 0.9f;
    arc.width = _data.reach * 0.3f;
    arc.arcDegree = _data.arcDegree * 2.0f;
    arc.tiltDegree = _data.arcTilt;
    arc.swing = swing;
    arc.color = _data.arcColor;
    arc.life = 0.2f;
    effects->PlaySlashArc(arc);
}

void EnemyAttackState::ApplyHit(Enemy& enemy, float time) {
    bool isFirstWindow = time >= _data.hitStart && time <= _data.hitEnd;
    bool isSecondWindow = _data.hitStart2 >= 0.0f && time >= _data.hitStart2 && time <= _data.hitEnd2;

    if (isSecondWindow && !_hasEnteredSecondHit) {
        _hasEnteredSecondHit = true;
        _hitList.clear();
    }
    if (!isFirstWindow && !isSecondWindow) return;

    auto* effects = EffectManager::Get();

    // 踏みつけは当たらなくても地面が揺れる 近くにいたことを知らせる
    bool isArea = _areaRadius > 0.0f;
    if (isFirstWindow && !_hasEnteredFirstHit) {
        _hasEnteredFirstHit = true;
        if (isArea && effects) {
            effects->Shake(_data.shake, 0.4f);
            effects->PlayShockwave(enemy.GetPosition(), _areaRadius, GetColorU8(255, 120, 60, 255));
        }
    }

    bool isFirstHit = _hitList.empty();
    int hits = isArea
        ? CombatSystem::ApplyArea(enemy, _areaRadius, _data, _hitList)
        : CombatSystem::ApplyMelee(enemy, _data, _hitList);

    if (hits <= 0 || !isFirstHit || !effects) return;

    effects->HitStop(_data.hitStop);
    if (!isArea && _data.shake > 0.0f) effects->Shake(_data.shake, 0.25f);
}
