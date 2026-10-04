#pragma once
#include "MonoBehaviour.h"
#include "EnemyData.h"
#include "AttackTokenPool.h"
#include "PhaseData.h"
#include "DxLib.h"
#include <deque>
#include <string>
#include <vector>

class Player;
class PlayerController;
class Enemy;

// フェーズの進行
// 敵の組み合わせをその場で組み立てて出し、全滅したら次へ進める
// 決まったフェーズごとに全回復 プレイヤーが倒れたら終わり
//
// 敵の並びを表に書くのではなく、フェーズ番号から強さの予算を決め、
// 出せる敵の中から予算に収まるまで選ぶ
// 数値を1つ変えるだけで難易度の伸び方を変えられる その数値は PhaseData にまとめてある
class PhaseDirector : public MonoBehaviour {
public:
    enum class Step {
        Announce,   // フェーズの番号を大きく出す 敵はもう出始めている
        Battle,
        Clear,
        Rest,       // 全回復して次の波まで休ませる
        GameOver,
    };

    // 進み方の数値 進め方の処理とフェーズの表示はここから読む
    PhaseData data;

private:
    static inline PhaseDirector* _instance = nullptr;

    Player* _player = nullptr;
    PlayerController* _controller = nullptr;
    AttackTokenPool _tokens;

    std::vector<Enemy*> _enemies;
    std::deque<EnemyKind> _spawnQueue;

    Step _step = Step::Announce;
    float _stepTimer = 0.0f;
    float _spawnTimer = 0.0f;
    int _phase = 1;
    int _killCount = 0;
    int _nextEnemyId = 0;
    int _bestPhase = 0;
    bool _isNewRecord = false;
    bool _isResultReady = false;
    std::string _currentBgm;

public:
    static PhaseDirector* Get() { return _instance; }

    ~PhaseDirector() override;

    void Initialize(Player* player, PlayerController* controller);
    void Update(float deltaTime) override;

    AttackTokenPool& GetTokens() { return _tokens; }

    Step GetStep() const { return _step; }
    float GetStepTimer() const { return _stepTimer; }
    int GetPhase() const { return _phase; }
    int GetKillCount() const { return _killCount; }
    int GetBestPhase() const { return _bestPhase; }
    bool IsNewRecord() const { return _isNewRecord; }
    bool IsResultReady() const { return _isResultReady; }
    const std::vector<Enemy*>& GetEnemies() const { return _enemies; }

    // まだ出ていない敵も含めた残り
    int GetRemainingEnemyCount() const;

    // 次の全回復まであと何フェーズ 今のフェーズを終えれば回復するなら 0
    int GetPhasesUntilHeal() const;

    // ----- 制作用 -----

    void DefeatAllEnemies();
    void SkipPhases(int count);
    void SpawnImmediately(EnemyKind kind);

private:
    void StartPhase(int phase);
    void EnterStep(Step step);
    void EnterGameOver();

    void UpdateSpawning(float deltaTime);
    void RemoveFinishedEnemies();

    Enemy* Spawn(EnemyKind kind);
    VECTOR ChooseSpawnPosition(const EnemyData& enemyData) const;
    std::vector<EnemyKind> BuildComposition(int phase) const;

    int GetAliveCount() const;
    int GetConcurrentLimit() const;
    int GetTokenCapacity() const;
    void UpdateBgm();
};
