#include "PhaseDirector.h"
#include "SaveData.h"
#include "Difficulty.h"
#include "Player.h"
#include "PlayerController.h"
#include "Enemy.h"
#include "EnemyFactory.h"
#include "EffectManager.h"
#include "StageBuilder.h"
#include "ArenaWall.h"
#include "SoundManager.h"
#include "Scene.h"
#include "GameObject.h"
#include <algorithm>
#include <cmath>

namespace {
    float RandomRange(float min, float max) {
        return min + (max - min) * (GetRand(1000) / 1000.0f);
    }
}

PhaseDirector::~PhaseDirector() {
    if (_instance == this) _instance = nullptr;
}

void PhaseDirector::Initialize(Player* player, PlayerController* controller) {
    _instance = this;
    _player = player;
    _controller = controller;
    _bestPhase = SaveData::LoadBestPhase(GameMode::Get());

    // 同時に攻撃してくる数は難易度で決める (Difficulty.cpp の表)
    const DifficultyData& difficulty = GameMode::GetData();
    data.tokenStepPhases = difficulty.tokenStepPhases;
    data.maxTokens = difficulty.maxTokens;
    data.healInterval = difficulty.healInterval;

    StartPhase(1);
}

void PhaseDirector::InitializePractice(Player* player, PlayerController* controller) {
    _instance = this;
    _player = player;
    _controller = controller;

    _tokens.SetCapacity(1);
    EnterStep(Step::Practice);
}

void PhaseDirector::Update(float deltaTime) {
    RemoveFinishedEnemies();

    // チュートリアルの間は片付けるだけ フェーズも勝ち負けも進めない
    if (_step == Step::Practice) return;

    _stepTimer += deltaTime;

    if (!IsFinished() && _player && _player->IsDead()) {
        EnterGameOver();
    }

    switch (_step) {
    case Step::Announce:
        UpdateSpawning(deltaTime);
        if (_stepTimer >= data.announceTime) EnterStep(Step::Battle);
        break;

    case Step::Battle:
        UpdateSpawning(deltaTime);
        if (_spawnQueue.empty() && GetAliveCount() == 0) {
            SoundManager::Instance().PlaySE("Common/system_counter");

            // 最後のフェーズを越えたら勝ち 全回復や次のフェーズへは進まない
            if (IsFinalPhase()) EnterVictory();
            else EnterStep(Step::Clear);
        }
        break;

    case Step::Clear:
        if (_stepTimer < data.clearTime) break;

        if (_phase % data.healInterval == 0) {
            EnterStep(Step::Rest);
            if (_player) _player->HealFull();
            if (auto* effects = EffectManager::Get()) effects->Shake(6.0f, 0.5f);
            SoundManager::Instance().PlayJingle("JINGLE_stageclear");
        }
        else {
            StartPhase(_phase + 1);
        }
        break;

    case Step::Rest:
        if (_stepTimer >= data.restTime) StartPhase(_phase + 1);
        break;

    case Step::GameOver:
        if (_stepTimer >= data.resultDelay) _isResultReady = true;
        break;

    case Step::Victory:
        // とどめの演出が落ち着いてから VICTORY を出し、少しおいて結果を出す
        if (!_isVictoryAnnounced && _stepTimer >= data.victoryDelay) AnnounceVictory();
        if (_stepTimer >= data.victoryDelay + data.victoryResultDelay) _isResultReady = true;
        break;

    case Step::Practice:
        // 上で戻っているので、ここへは来ない
        break;
    }
}

int PhaseDirector::GetRemainingEnemyCount() const {
    return GetAliveCount() + static_cast<int>(_spawnQueue.size());
}

int PhaseDirector::GetPhasesUntilHeal() const {
    int remainder = _phase % data.healInterval;
    return (remainder == 0) ? 0 : data.healInterval - remainder;
}

int PhaseDirector::GetPhasesUntilVictory() const {
    if (data.finalPhase <= 0) return -1;

    int left = data.finalPhase - _phase;
    return (left > 0) ? left : 0;
}

void PhaseDirector::DefeatAllEnemies() {
    _spawnQueue.clear();

    // 体力がないので、真ん中から外へ向けて場外へ飛ばして倒す
    VECTOR center = StageBuilder::GetArenaCenter();
    for (Enemy* enemy : _enemies) {
        if (enemy->IsDead()) continue;

        VECTOR away = VSub(enemy->GetPosition(), center);
        away.y = 0.0f;
        float length = VSize(away);
        VECTOR direction = (length > 1.0f) ? VScale(away, 1.0f / length) : VGet(1.0f, 0.0f, 0.0f);
        enemy->Defeat(VScale(direction, ArenaWall::BREAK_FLY_MIN_SPEED));
    }
}

void PhaseDirector::SkipPhases(int count) {
    if (IsFinished()) return;

    DefeatAllEnemies();

    // 最後のフェーズより先へは飛ばさない 最後のフェーズで使えば、全滅させたのでそのまま勝ちになる
    int target = _phase + count;
    if (data.finalPhase > 0 && target > data.finalPhase) target = data.finalPhase;
    if (target > _phase) StartPhase(target);
}

void PhaseDirector::SpawnImmediately(EnemyKind kind) {
    if (IsFinished()) return;
    Spawn(kind);
}

void PhaseDirector::RemoveAllEnemies() {
    _spawnQueue.clear();

    // 実際に消えるのはフレームの最後 持っていた番は、消えるときに AI が返す
    for (Enemy* enemy : _enemies) {
        Scene::Instance().Destroy(enemy->gameObject);
    }
    _enemies.clear();
}

void PhaseDirector::StartPhase(int phase) {
    _phase = phase;

    std::vector<EnemyKind> composition = BuildComposition(phase);
    _spawnQueue.assign(composition.begin(), composition.end());

    // 番号を出してから少し待って出し始める (PhaseData の spawnStartDelay)
    _spawnTimer = data.spawnStartDelay;

    _tokens.SetCapacity(GetTokenCapacity());
    UpdateBgm();

    SoundManager::Instance().PlaySE("Common/system_enter");
    EnterStep(Step::Announce);
}

void PhaseDirector::EnterStep(Step step) {
    _step = step;
    _stepTimer = 0.0f;
}

void PhaseDirector::EnterGameOver() {
    EnterStep(Step::GameOver);
    _spawnQueue.clear();

    if (_controller) _controller->isInputLocked = true;

    // 倒れたフェーズまでたどり着いた、と数える
    SaveRecord();

    SoundManager::Instance().StopAllBGM(1.0f);
    SoundManager::Instance().PlayJingle("JINGLE_gameover");
    _currentBgm.clear();
}

void PhaseDirector::EnterVictory() {
    EnterStep(Step::Victory);
    _isVictoryAnnounced = false;

    // 勝ったあとは動かさない 結果が出たらボタンでタイトルへ戻る
    if (_controller) _controller->isInputLocked = true;

    // 最後のフェーズまでたどり着いた、と数える
    SaveRecord();
}

void PhaseDirector::AnnounceVictory() {
    _isVictoryAnnounced = true;

    SoundManager::Instance().StopAllBGM(1.0f);
    SoundManager::Instance().PlayJingle("JINGLE_stageclear");
    _currentBgm.clear();

    // 画面の演出は、知らせを受けた Observer が出す (GameScene.cpp で登録)
    VictoryEvent event;
    event.phase = _phase;
    if (_player) event.playerPosition = _player->GetPosition();
    _victoryEvents.Notify(event);
}

void PhaseDirector::SaveRecord() {
    _isNewRecord = _phase > _bestPhase;
    if (!_isNewRecord) return;

    _bestPhase = _phase;
    SaveData::SaveBestPhase(GameMode::Get(), _phase);
}

bool PhaseDirector::IsFinished() const {
    return _step == Step::GameOver || _step == Step::Victory;
}

bool PhaseDirector::IsFinalPhase() const {
    return data.finalPhase > 0 && _phase >= data.finalPhase;
}

void PhaseDirector::UpdateSpawning(float deltaTime) {
    _spawnTimer -= deltaTime;
    if (_spawnTimer > 0.0f || _spawnQueue.empty()) return;
    if (GetAliveCount() >= GetConcurrentLimit()) return;

    EnemyKind kind = _spawnQueue.front();
    _spawnQueue.pop_front();
    Spawn(kind);

    // 一度に湧くと、どこから来たのか分からなくなるので少しずつ出す
    _spawnTimer = data.spawnInterval;
}

void PhaseDirector::RemoveFinishedEnemies() {
    bool hasNewDefeat = false;
    VECTOR lastDefeatPosition = VGet(0.0f, 0.0f, 0.0f);

    for (auto it = _enemies.begin(); it != _enemies.end();) {
        Enemy* enemy = *it;

        if (enemy->TryCountDefeat()) {
            _killCount++;
            hasNewDefeat = true;
            lastDefeatPosition = enemy->GetPosition();
        }

        if (enemy->IsReadyToRemove()) {
            // 実際に消えるのはフレームの最後 それまでは一覧から外すだけ
            Scene::Instance().Destroy(enemy->gameObject);
            it = _enemies.erase(it);
        }
        else {
            ++it;
        }
    }

    // 波の最後の1体を倒した瞬間を大きく見せる 無双の区切りの手応え
    bool isFighting = _step == Step::Announce || _step == Step::Battle;
    if (hasNewDefeat && isFighting && _spawnQueue.empty() && GetAliveCount() == 0) {
        // 吹き飛んで宙にいることが多いので、輪は真下の地面に出す
        StageBuilder::FindGroundHeight(lastDefeatPosition.x, lastDefeatPosition.z, lastDefeatPosition.y);
        if (auto* effects = EffectManager::Get()) effects->PlayFinalBlow(lastDefeatPosition);
    }
}

Enemy* PhaseDirector::Spawn(EnemyKind kind) {
    const EnemyData& enemyData = EnemyDatabase::Get(kind);
    return SpawnAt(enemyData, ChooseSpawnPosition(enemyData));
}

Enemy* PhaseDirector::SpawnAt(const EnemyData& enemyData, VECTOR position) {
    Enemy* enemy = EnemyFactory::Create(enemyData, position, _player, _nextEnemyId++);
    if (!enemy) return nullptr;

    _enemies.push_back(enemy);

    if (auto* effects = EffectManager::Get()) {
        // 飛ぶ敵も、現れる印は地面に出す
        VECTOR ground = position;
        StageBuilder::FindGroundHeight(position.x, position.z, ground.y);
        effects->PlaySpawn(ground);
    }
    SoundManager::Instance().PlaySE("Common/enemy_Nifram", 0.6f);
    return enemy;
}

VECTOR PhaseDirector::ChooseSpawnPosition(const EnemyData& enemyData) const {
    VECTOR arenaCenter = StageBuilder::GetArenaCenter();
    VECTOR playerPosition = _player ? _player->GetPosition() : arenaCenter;
    float limit = StageBuilder::ARENA_RADIUS - 150.0f;
    float height = enemyData.isFlying ? enemyData.hoverHeight : 40.0f;

    constexpr int ATTEMPTS = 8;
    for (int i = 0; i < ATTEMPTS; ++i) {
        float angle = RandomRange(0.0f, DX_TWO_PI_F);
        float distance = RandomRange(data.spawnDistanceMin, data.spawnDistanceMax);
        VECTOR position = VAdd(playerPosition, VGet(cosf(angle) * distance, 0.0f, sinf(angle) * distance));

        // 戦える範囲の外に出たら端まで戻す
        VECTOR offset = VSub(position, arenaCenter);
        offset.y = 0.0f;
        float fromCenter = VSize(offset);
        if (fromCenter > limit) {
            position = VAdd(arenaCenter, VScale(offset, limit / fromCenter));
        }

        // 端へ戻したせいでプレイヤーの目の前になったら選び直す
        VECTOR gap = VSub(position, playerPosition);
        gap.y = 0.0f;
        if (VSize(gap) < data.spawnMinGap) continue;

        float groundY = 0.0f;
        if (!StageBuilder::FindGroundHeight(position.x, position.z, groundY)) continue;

        position.y = groundY + height;
        return position;
    }

    // どこも見つからなければ、真ん中を挟んでプレイヤーの反対側に出す
    VECTOR away = VSub(arenaCenter, playerPosition);
    away.y = 0.0f;
    float awayLength = VSize(away);
    VECTOR fallback = (awayLength > 1.0f)
        ? VAdd(arenaCenter, VScale(away, 900.0f / awayLength))
        : VAdd(arenaCenter, VGet(900.0f, 0.0f, 0.0f));

    float groundY = playerPosition.y;
    StageBuilder::FindGroundHeight(fallback.x, fallback.z, groundY);
    fallback.y = groundY + height;
    return fallback;
}

std::vector<EnemyKind> PhaseDirector::BuildComposition(int phase) const {
    float budget = data.baseBudget + phase * data.budgetPerPhase;

    std::vector<EnemyKind> result;
    int counts[static_cast<int>(EnemyKind::Count)] = {};

    auto add = [&](EnemyKind kind) {
        result.push_back(kind);
        counts[static_cast<int>(kind)]++;
        budget -= EnemyDatabase::Get(kind).cost;
    };

    // 新しく出てくる敵は、出始めのフェーズで必ず先頭に入れる 何が増えたのか覚えてもらうため
    for (int i = 0; i < static_cast<int>(EnemyKind::Count); ++i) {
        EnemyKind kind = static_cast<EnemyKind>(i);
        if (EnemyDatabase::Get(kind).unlockPhase == phase) add(kind);
    }

    while (budget > 0.0f) {
        // 出せる敵の中から、選ばれやすさに応じて1体選ぶ
        std::vector<EnemyKind> candidates;
        int totalWeight = 0;
        for (int i = 0; i < static_cast<int>(EnemyKind::Count); ++i) {
            EnemyKind kind = static_cast<EnemyKind>(i);
            const EnemyData& enemyData = EnemyDatabase::Get(kind);

            bool isAvailable = enemyData.unlockPhase <= phase
                && enemyData.cost <= budget
                && counts[i] < enemyData.maxPerPhase;
            if (!isAvailable) continue;

            candidates.push_back(kind);
            totalWeight += enemyData.pickWeight;
        }
        if (candidates.empty() || totalWeight <= 0) break;

        int roll = GetRand(totalWeight - 1);
        for (EnemyKind kind : candidates) {
            roll -= EnemyDatabase::Get(kind).pickWeight;
            if (roll < 0) {
                add(kind);
                break;
            }
        }
    }

    return result;
}

int PhaseDirector::GetAliveCount() const {
    int count = 0;
    for (const Enemy* enemy : _enemies) {
        if (!enemy->IsDead()) count++;
    }
    return count;
}

// std::min は DxLib が読む Windows.h の min マクロとぶつかって使えないので、比べて書く
int PhaseDirector::GetConcurrentLimit() const {
    int limit = data.minConcurrent + _phase / 2;
    return (limit < data.maxConcurrent) ? limit : data.maxConcurrent;
}

int PhaseDirector::GetTokenCapacity() const {
    // フェーズが進むほど同時に殴ってくる数を増やす
    int capacity = 1 + (_phase - 1) / data.tokenStepPhases;
    return (capacity < data.maxTokens) ? capacity : data.maxTokens;
}

void PhaseDirector::UpdateBgm() {
    const char* bgm = "BGM_stg0";
    if (_phase >= data.bossBgmPhase) bgm = "BGM_boss";
    else if (_phase >= data.stage2BgmPhase) bgm = "BGM_stg1";

    if (_currentBgm == bgm) return;

    SoundManager::Instance().CrossfadeBGM(bgm, data.bgmCrossfadeTime);
    _currentBgm = bgm;
}
