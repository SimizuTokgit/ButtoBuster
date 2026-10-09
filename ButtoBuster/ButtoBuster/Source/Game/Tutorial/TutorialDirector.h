#pragma once
#include "MonoBehaviour.h"
#include "Observer.h"
#include "PlayerChargeEvent.h"
#include "JustDodgeEvent.h"
#include "WallBreakEvent.h"
#include "SpecialEvent.h"
#include "TutorialSteps.h"
#include "DxLib.h"
#include <vector>

class Player;
class PlayerController;
class CameraFollow;
class PhaseDirector;
class Enemy;
struct EnemyData;

// チュートリアルの進め役
// 段の表 (TutorialSteps.cpp) を上から 1 段ずつ出し、言われたことができたら次の段へ進める
// 練習の相手は段ごとに入れ替える 出すのと片付けるのは、練習の段 (Practice) の PhaseDirector に頼む
// 最後の段で決定を押すか、Enter / Start を長押しすると本番 (GameScene) へ
// 画面の表示は TutorialScreen が、ここから読んで出す
class TutorialDirector : public MonoBehaviour,
    public Observer<PlayerChargeEvent>,
    public Observer<JustDodgeEvent>,
    public Observer<WallBreakEvent>,
    public Observer<SpecialEvent> {
public:
    // できてから「OK!」を出しておく秒数 そのあと次の段へ
    static constexpr float CLEAR_TIME = 1.2f;

    // 段が始まってから動けるようになるまでの秒数 読む段で押した決定が、次の段でジャンプにならないように
    static constexpr float START_DELAY = 0.3f;

    // 本番へ飛ばすのに Enter / Start を押し続ける秒数
    static constexpr float SKIP_HOLD_TIME = 1.5f;

    // 「動いてみよう」で歩く長さ cm
    static constexpr float MOVE_DISTANCE = 1000.0f;

    // 「見回してみよう」で、手で回す角度の合計 度
    static constexpr float LOOK_DEGREES = 180.0f;

    // 「必殺技」の段の始めに、バースターゲージをここまで溜めておく 0〜1
    // 練習台をまとめて斬れば 1 回のコンボでも溜まるが、すぐ撃てるように最初から 7 割にしておく
    static constexpr float SPECIAL_START_RATIO = 0.7f;

    // 練習の相手を出す、プレイヤーの前 (カメラの向き) の距離 cm 大きい Golem はこの倍
    static constexpr float PARTNER_DISTANCE = 450.0f;

    // 練習の相手が壁の近くに出そうなときに、体の太さとこの分だけ内側へ寄せる cm
    static constexpr float PARTNER_WALL_MARGIN = 200.0f;

    // まとめて出す練習台 (TutorialPartner::Crowd) の数と、プレイヤーの前のどれくらいの広さに並べるか cm
    static constexpr int CROWD_COUNT = 6;
    static constexpr float CROWD_RADIUS = 220.0f;

    // 練習の相手が倒れて場外へ消えたら、出し直すまでの秒数 まとめて出した練習台は、全部消えてから出し直す
    static constexpr float RESPAWN_DELAY = 1.5f;

private:
    Player* _player = nullptr;
    PlayerController* _controller = nullptr;
    CameraFollow* _camera = nullptr;
    PhaseDirector* _phases = nullptr;

    int _stepIndex = 0;
    float _stepTimer = 0.0f;        // 今の段が始まってからの実時間
    int _progress = 0;              // 今の段で数えた回数
    bool _isCleared = false;        // できて「OK!」を出している間
    float _clearTimer = 0.0f;
    bool _isLeaving = false;        // 本番へ移ると決めた

    // Enter / Start を押し続けている秒数
    float _skipTimer = 0.0f;

    // できたかどうかを見るための、前のフレームの様子
    VECTOR _lastPosition = VGet(0.0f, 0.0f, 0.0f);
    float _lastYaw = 0.0f;
    float _movedDistance = 0.0f;
    float _lookedDegrees = 0.0f;
    bool _wasJumping = false;
    bool _wasSlamming = false;
    float _lastDodgeStock = 0.0f;
    int _guardCountAtStart = 0;

    // 練習の相手
    Enemy* _partner = nullptr;
    TutorialPartner _partnerKind = TutorialPartner::None;
    std::vector<Enemy*> _crowd;         // まとめて出した練習台 _partner はこの先頭
    float _respawnTimer = 0.0f;
    bool _wasPartnerSwinging = false;   // 前のフレームに振っていたか
    int _hitCountAtSwing = 0;           // 振り始めたときに、プレイヤーが食らっていた回数
    bool _wasPartnerHovering = false;   // 前のフレームに飛んでいたか

public:
    void Initialize(Player* player, PlayerController* controller, CameraFollow* camera, PhaseDirector* phases);
    void Update(float deltaTime) override;

    void OnNotify(const PlayerChargeEvent& event) override;
    void OnNotify(const JustDodgeEvent& event) override;
    void OnNotify(const WallBreakEvent& event) override;

    // 必殺技の雷が落ちたら、必殺技の段はできた
    void OnNotify(const SpecialEvent& event) override;

    // ----- 画面 (TutorialScreen) が読む -----

    const TutorialStep& GetStep() const { return TutorialSteps::Get(_stepIndex); }
    int GetStepNumber() const { return _stepIndex + 1; }
    int GetStepCount() const { return TutorialSteps::GetCount(); }
    float GetStepTime() const { return _stepTimer; }
    bool IsCleared() const { return _isCleared; }

    // 読むだけの段か (動けず、決定で次へ進む) と、その段で決定を待っているか
    bool IsReadStep() const { return IsReadGoal(GetStep().goal); }
    bool IsWaitingConfirm() const;

    // 回数を数える段か 画面は「1 / 3」のように出す
    bool IsCounted() const;
    int GetProgress() const { return _progress; }

    // 進み具合をバーで出す段か (歩いた長さ 回した角度 相手の吹っ飛ばされ値) と、その割合 0〜1
    bool HasProgressBar() const;
    float GetProgressRatio() const;

    // 長押しでスキップの溜まり具合 0〜1
    float GetSkipRatio() const;

private:
    void BeginStep();
    void NextStep();
    void Clear();
    void GoToGame();

    void UpdateSkip();
    void UpdateGoal(const TutorialStep& step);
    void SetProgress(int progress);
    void AddProgress(int amount) { SetProgress(_progress + amount); }

    // 練習の相手
    void RefreshPartner();
    void KeepPartner(float deltaTime);
    void SpawnPartner();
    Enemy* SpawnPartnerAt(const EnemyData& data, VECTOR position);
    void ResetPartnerWatch();

    // 相手が振り終えたときに、振っている間に食らっていなければ「かわした」と数える
    void WatchSwings();
    bool IsPartnerSwinging() const;
    bool IsPartnerAtRest() const;

    float GetCameraYaw() const;
    static bool IsReadGoal(TutorialGoal goal);
};
