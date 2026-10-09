#pragma once
#include "MonoBehaviour.h"
#include "EnemyData.h"
#include "AttackTokenPool.h"
#include "PhaseData.h"
#include "Observer.h"
#include "VictoryEvent.h"
#include "DxLib.h"
#include <deque>
#include <string>
#include <vector>

class Player;
class PlayerController;
class Enemy;

// フェーズの進行
// 敵の組み合わせをその場で組み立てて出し、全滅したら次へ進める
// 決まったフェーズごとに全回復 (吹っ飛ばされ値を 0 に戻す) プレイヤーが壁を割られたら終わり
// 最後のフェーズ (PhaseData の finalPhase) の敵を全部倒したら勝ち
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
        Victory,    // 最後のフェーズを越えた とどめの演出のあと VICTORY と結果を出す
        Practice,   // チュートリアル フェーズを進めず、頼まれた敵だけを出して、場外へ消えたものを片付ける
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

    Subject<VictoryEvent> _victoryEvents;

    Step _step = Step::Announce;
    float _stepTimer = 0.0f;
    float _spawnTimer = 0.0f;
    int _phase = 1;

    // このフェーズが始まったときに、プレイヤーが食らっていた回数 越えたときの声を選ぶのに使う
    int _hitCountAtPhaseStart = 0;
    int _killCount = 0;
    int _nextEnemyId = 0;
    int _bestPhase = 0;
    bool _isNewRecord = false;
    bool _isResultReady = false;

    // 勝ったあと、VICTORY を出して知らせたか
    bool _isVictoryAnnounced = false;

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

    // 勝ちまであと何フェーズ 今のフェーズを終えれば勝ちなら 0 勝ちが無いなら -1
    int GetPhasesUntilVictory() const;

    // 勝ったことを知らせる先 VICTORY を出す瞬間に知らせる 演出はここに Observer として登録する
    Subject<VictoryEvent>& GetVictoryEvents() { return _victoryEvents; }

    // ----- チュートリアル -----

    // フェーズを始めずに練習の段 (Practice) にする 敵は SpawnAt で頼まれたものだけを出す 攻撃の番は 1 体ずつ
    void InitializePractice(Player* player, PlayerController* controller);

    // 決めた場所に敵を出す 数値を書き換えた敵も出せる 敵は enemyData を指したまま持つので、消えるまで残しておくこと
    Enemy* SpawnAt(const EnemyData& enemyData, VECTOR position);

    // 今いる敵をすぐに全部消す 練習の相手を入れ替えるときに使う
    void RemoveAllEnemies();

    // ----- 制作用 -----

    void DefeatAllEnemies();
    void SkipPhases(int count);
    void SpawnImmediately(EnemyKind kind);

private:
    void StartPhase(int phase);
    // フェーズを越えたときのプレイヤーの声 食らわなかった 危ない ふつう で変える
    void PlayPhaseClearVoice();
    void EnterStep(Step step);
    void EnterGameOver();
    void EnterVictory();
    void AnnounceVictory();

    // たどり着いたフェーズが最高記録を超えていたら残す
    void SaveRecord();

    // 負けか勝ちで、もう戦わない
    bool IsFinished() const;

    // 今のフェーズを越えたら勝ちか
    bool IsFinalPhase() const;

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
