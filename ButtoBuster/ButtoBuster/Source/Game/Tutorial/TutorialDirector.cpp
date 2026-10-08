#include "TutorialDirector.h"
#include "GameObject.h"
#include "Player.h"
#include "PlayerController.h"
#include "PlayerJumpState.h"
#include "PlayerAirSlamState.h"
#include "CameraFollow.h"
#include "PhaseDirector.h"
#include "Enemy.h"
#include "EnemyAI.h"
#include "EnemyData.h"
#include "EnemyAttackState.h"
#include "EnemyIdleState.h"
#include "EnemyMoveState.h"
#include "ArenaWall.h"
#include "StageBuilder.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "InputSystem.h"
#include "Time.h"
#include <algorithm>
#include <cmath>

namespace {
    // ----- 練習の相手の数値 -----
    // 本番の数値 (EnemyData.cpp) を写して、練習に合うところだけ変える 本番の敵は変わらない

    // 普通の斬りだけを振る Goblin ガードを見ても大振りに変えないよう、賢さは 1 にする
    EnemyData MakeSlasher() {
        EnemyData data = EnemyDatabase::Get(EnemyKind::Goblin);
        data.intelligence = 1;
        data.heavyChance = 0.0f;
        return data;
    }

    // 大振りだけを振る Goblin 賢さ 2 から大振りを振るので 2 にする
    EnemyData MakeHeavy() {
        EnemyData data = EnemyDatabase::Get(EnemyKind::Goblin);
        data.intelligence = 2;
        data.heavyChance = 1.0f;
        return data;
    }

    // 踏みつけだけを出す Golem
    EnemyData MakeStomper() {
        EnemyData data = EnemyDatabase::Get(EnemyKind::Golem);
        data.heavyChance = 1.0f;
        return data;
    }

    // 敵は数値を指したまま持つので、ずっと残る static に置く
    const EnemyData& GetPartnerData(TutorialPartner partner) {
        static const EnemyData slasher = MakeSlasher();
        static const EnemyData heavy = MakeHeavy();
        static const EnemyData stomper = MakeStomper();

        switch (partner) {
        case TutorialPartner::Slasher: return slasher;
        case TutorialPartner::Heavy:   return heavy;
        case TutorialPartner::Bee:     return EnemyDatabase::Get(EnemyKind::Bee);
        case TutorialPartner::Golem:   return stomper;
        default:                       return EnemyDatabase::Get(EnemyKind::Goblin);
        }
    }

    // 攻撃してこない相手 (EnemyAI の isPassive)
    bool IsPassivePartner(TutorialPartner partner) {
        return partner == TutorialPartner::Dummy || partner == TutorialPartner::Bee;
    }
}

void TutorialDirector::Initialize(Player* player, PlayerController* controller, CameraFollow* camera, PhaseDirector* phases) {
    _player = player;
    _controller = controller;
    _camera = camera;
    _phases = phases;
    if (!_player || !_phases) return;

    // 練習の間は負けない 攻撃は受けるが、壁を割られずに跳ね返る
    _player->canLose = false;

    // 溜め、ジャスト回避、壁割りは、知らせを受けて数える
    _player->GetChargeEvents().AddObserver(this);
    _player->GetJustDodgeEvents().AddObserver(this);
    ArenaWall::GetBreakEvents().AddObserver(this);

    _stepIndex = 0;
    BeginStep();
}

void TutorialDirector::Update(float deltaTime) {
    if (!_player || !_phases) return;

    UpdateSkip();
    if (_isLeaving) return;

    // 段の時間は実時間で数える ヒットストップや反撃のスローで止まらないように
    float realDeltaTime = Time::UnscaledDeltaTime();
    _stepTimer += realDeltaTime;

    RefreshPartner();
    KeepPartner(realDeltaTime);

    const TutorialStep& step = GetStep();
    bool isRead = IsReadGoal(step.goal);

    // 読む段は動けない 動く段も、始まってすぐは動けない
    if (_controller) _controller->isInputLocked = isRead || _stepTimer < START_DELAY;

    if (_isCleared) {
        _clearTimer += realDeltaTime;
        if (_clearTimer >= CLEAR_TIME) NextStep();
        return;
    }

    if (isRead) {
        if (IsWaitingConfirm() && InputSystem::Instance().ConfirmPressed()) {
            SoundManager::Instance().PlaySE("Common/system_enter");
            if (step.goal == TutorialGoal::Finish) GoToGame();
            else NextStep();
        }
        return;
    }

    UpdateGoal(step);
}

void TutorialDirector::OnNotify(const PlayerChargeEvent& event) {
    if (_isCleared || _isLeaving) return;

    // count 段階以上溜めて振ったらできた
    const TutorialStep& step = GetStep();
    if (step.goal != TutorialGoal::Charge || event.type != PlayerChargeEvent::Type::Release) return;
    if (event.level >= step.count) SetProgress(step.count);
}

void TutorialDirector::OnNotify(const JustDodgeEvent& event) {
    if (_isCleared || _isLeaving) return;
    if (GetStep().goal == TutorialGoal::JustDodge) AddProgress(1);
}

void TutorialDirector::OnNotify(const WallBreakEvent& event) {
    if (_isCleared || _isLeaving || event.isPlayer) return;
    if (GetStep().goal == TutorialGoal::BreakWall) AddProgress(1);
}

bool TutorialDirector::IsWaitingConfirm() const {
    return IsReadGoal(GetStep().goal) && !_isCleared && !_isLeaving && _stepTimer >= START_DELAY;
}

bool TutorialDirector::IsCounted() const {
    switch (GetStep().goal) {
    case TutorialGoal::Jump:
    case TutorialGoal::Combo:
    case TutorialGoal::AirSlam:
    case TutorialGoal::Guard:
    case TutorialGoal::Dodge:
    case TutorialGoal::AvoidHeavy:
    case TutorialGoal::JustDodge:
    case TutorialGoal::BreakWall:
    case TutorialGoal::DropBee:
    case TutorialGoal::AvoidStomp:
        return true;
    default:
        return false;
    }
}

bool TutorialDirector::HasProgressBar() const {
    TutorialGoal goal = GetStep().goal;
    return goal == TutorialGoal::Move || goal == TutorialGoal::Look || goal == TutorialGoal::HeatEnemy;
}

float TutorialDirector::GetProgressRatio() const {
    if (_isCleared) return 1.0f;

    float ratio = 0.0f;
    switch (GetStep().goal) {
    case TutorialGoal::Move:
        ratio = _movedDistance / MOVE_DISTANCE;
        break;
    case TutorialGoal::Look:
        ratio = _lookedDegrees / LOOK_DEGREES;
        break;
    case TutorialGoal::HeatEnemy:
        if (_partner && !_partner->IsDead()) ratio = _partner->GetBlowRatio();
        break;
    default:
        break;
    }
    return (ratio < 1.0f) ? ratio : 1.0f;
}

float TutorialDirector::GetSkipRatio() const {
    float ratio = _skipTimer / SKIP_HOLD_TIME;
    return (ratio < 1.0f) ? ratio : 1.0f;
}

void TutorialDirector::BeginStep() {
    const TutorialStep& step = GetStep();
    _stepTimer = 0.0f;
    _progress = 0;
    _isCleared = false;
    _clearTimer = 0.0f;
    if (_controller) _controller->isInputLocked = true;

    // できたかどうかを見る起点を、段が始まった今にそろえる
    auto& states = _player->GetStates();
    _lastPosition = _player->GetPosition();
    _lastYaw = GetCameraYaw();
    _movedDistance = 0.0f;
    _lookedDegrees = 0.0f;
    _wasJumping = states.IsIn<PlayerJumpState>();
    _wasSlamming = states.IsIn<PlayerAirSlamState>();
    _lastDodgeStock = _player->GetDodgeStock();
    _guardCountAtStart = _player->GetGuardCount();

    // 前の段で食らった分は持ち越さない 自分の吹っ飛ばされ値を見せる段だけは、わざと許容値まで溜める
    if (step.goal == TutorialGoal::OwnHeat) _player->SetBlowRatio(1.0f);
    else _player->ResetBlow();

    // 練習の相手を入れ替える 前の段と同じ相手なら、そのまま使う
    if (step.partner != _partnerKind) {
        _phases->RemoveAllEnemies();
        _partner = nullptr;
        _partnerKind = step.partner;
        _respawnTimer = 0.0f;
        if (_partnerKind != TutorialPartner::None) SpawnPartner();
    }
    ResetPartnerWatch();
}

void TutorialDirector::NextStep() {
    // 自分の吹っ飛ばされ値を見せた段のあとは、全回復の演出で 0 に戻す
    if (GetStep().goal == TutorialGoal::OwnHeat) _player->HealFull();

    _stepIndex++;
    if (_stepIndex >= GetStepCount()) {
        GoToGame();
        return;
    }
    BeginStep();
}

void TutorialDirector::Clear() {
    if (_isCleared) return;

    _isCleared = true;
    _clearTimer = 0.0f;
    SoundManager::Instance().PlaySE("Common/system_counter");
}

void TutorialDirector::GoToGame() {
    if (_isLeaving) return;

    _isLeaving = true;
    if (_controller) _controller->isInputLocked = true;
    SceneManager::Instance().RequestLoadScene<GameScene>();
}

void TutorialDirector::UpdateSkip() {
    if (_isLeaving) return;

    // 押している間だけ溜まり、離したら最初から
    if (InputSystem::Instance().SkipHeld()) _skipTimer += Time::UnscaledDeltaTime();
    else _skipTimer = 0.0f;

    if (_skipTimer >= SKIP_HOLD_TIME) {
        SoundManager::Instance().PlaySE("Common/system_enter");
        GoToGame();
    }
}

void TutorialDirector::UpdateGoal(const TutorialStep& step) {
    auto& states = _player->GetStates();

    switch (step.goal) {
    case TutorialGoal::Move: {
        VECTOR position = _player->GetPosition();
        VECTOR moved = VSub(position, _lastPosition);
        moved.y = 0.0f;
        _lastPosition = position;

        _movedDistance += VSize(moved);
        if (_movedDistance >= MOVE_DISTANCE) Clear();
        break;
    }

    case TutorialGoal::Look: {
        // 手で回した分だけ数える 歩いてカメラが付いてきた分は数えない
        float yaw = GetCameraYaw();
        float change = yaw - _lastYaw;
        while (change > 180.0f) change -= 360.0f;
        while (change < -180.0f) change += 360.0f;
        _lastYaw = yaw;

        if (_controller && _controller->HasViewInput()) _lookedDegrees += fabsf(change);
        if (_lookedDegrees >= LOOK_DEGREES) Clear();
        break;
    }

    case TutorialGoal::Jump: {
        bool isJumping = states.IsIn<PlayerJumpState>();
        if (isJumping && !_wasJumping) AddProgress(1);
        _wasJumping = isJumping;
        break;
    }

    case TutorialGoal::Combo:
        // 続けて当てた数 食らったり途切れたりしたら数え直し
        SetProgress(_player->GetCombo());
        break;

    case TutorialGoal::AirSlam: {
        bool isSlamming = states.IsIn<PlayerAirSlamState>();
        if (isSlamming && !_wasSlamming) AddProgress(1);
        _wasSlamming = isSlamming;
        break;
    }

    case TutorialGoal::Guard:
        SetProgress(_player->GetGuardCount() - _guardCountAtStart);
        break;

    case TutorialGoal::Dodge: {
        // 回避すると残りが 1 回分減る 戻る途中の少しずつの増え方とは見分けられる
        float stock = _player->GetDodgeStock();
        if (stock < _lastDodgeStock - 0.5f) AddProgress(1);
        _lastDodgeStock = stock;
        break;
    }

    case TutorialGoal::AvoidHeavy:
    case TutorialGoal::AvoidStomp:
        WatchSwings();
        break;

    case TutorialGoal::HeatEnemy:
        if (_partner && !_partner->IsDead() && _partner->GetBlowRatio() >= 1.0f) Clear();
        break;

    case TutorialGoal::DropBee: {
        // 飛んでいた Bee が落ちたらできた
        bool isHovering = _partner && !_partner->IsDead() && _partner->IsHovering();
        if (_partner && _wasPartnerHovering && !isHovering) AddProgress(1);
        _wasPartnerHovering = isHovering;
        break;
    }

    default:
        // 溜め斬り、ジャスト回避、壁割りは、知らせ (OnNotify) で数える
        break;
    }
}

void TutorialDirector::SetProgress(int progress) {
    if (_isCleared) return;

    int count = GetStep().count;
    _progress = (progress < count) ? progress : count;
    if (_progress >= count) Clear();
}

void TutorialDirector::RefreshPartner() {
    if (!_partner) return;

    // 場外へ飛んで消えた相手は、PhaseDirector の一覧から外れる 外れたら手放す
    const auto& enemies = _phases->GetEnemies();
    if (std::find(enemies.begin(), enemies.end(), _partner) == enemies.end()) {
        _partner = nullptr;
        _respawnTimer = 0.0f;
    }
}

void TutorialDirector::KeepPartner(float deltaTime) {
    if (_partnerKind == TutorialPartner::None || _partner || _isCleared) return;

    // 倒れて消えたら、少し待って出し直す
    _respawnTimer += deltaTime;
    if (_respawnTimer < RESPAWN_DELAY) return;

    _respawnTimer = 0.0f;
    SpawnPartner();
    ResetPartnerWatch();
}

void TutorialDirector::SpawnPartner() {
    const EnemyData& data = GetPartnerData(_partnerKind);

    // カメラの向いている先、プレイヤーの前に出す 出たところが見えるように
    VECTOR forward = _camera ? _camera->GetGroundForward() : _player->GetForward();
    float distance = (_partnerKind == TutorialPartner::Golem) ? PARTNER_DISTANCE * 2.0f : PARTNER_DISTANCE;
    VECTOR position = VAdd(_player->GetPosition(), VScale(forward, distance));
    position = ArenaWall::ClampInside(position, data.bodyRadius + PARTNER_WALL_MARGIN);

    float groundY = _player->GetPosition().y;
    StageBuilder::FindGroundHeight(position.x, position.z, groundY);
    position.y = groundY + (data.isFlying ? data.hoverHeight : 40.0f);

    _partner = _phases->SpawnAt(data, position);
    if (!_partner) return;

    // 練習相手は難易度に関係なく、上の数値の通りに動かす
    if (auto* ai = _partner->GetComponent<EnemyAI>()) {
        ai->followsDifficulty = false;
        ai->isPassive = IsPassivePartner(_partnerKind);
    }
}

void TutorialDirector::ResetPartnerWatch() {
    _wasPartnerSwinging = IsPartnerSwinging();
    _hitCountAtSwing = _player->GetHitCount();
    _wasPartnerHovering = _partner && !_partner->IsDead() && _partner->IsHovering();
}

void TutorialDirector::WatchSwings() {
    bool isSwinging = IsPartnerSwinging();
    if (isSwinging && !_wasPartnerSwinging) _hitCountAtSwing = _player->GetHitCount();

    // 振り終えて構えに戻ったときに、振っている間に食らっていなければかわした
    // 振っている途中で斬られて止まったときは数えない
    if (!isSwinging && _wasPartnerSwinging && IsPartnerAtRest()) {
        if (_player->GetHitCount() == _hitCountAtSwing) AddProgress(1);
    }
    _wasPartnerSwinging = isSwinging;
}

bool TutorialDirector::IsPartnerSwinging() const {
    return _partner && !_partner->IsDead() && _partner->GetStates().IsIn<EnemyAttackState>();
}

bool TutorialDirector::IsPartnerAtRest() const {
    if (!_partner || _partner->IsDead()) return false;

    auto& states = _partner->GetStates();
    return states.IsIn<EnemyIdleState>() || states.IsIn<EnemyMoveState>();
}

float TutorialDirector::GetCameraYaw() const {
    if (!_camera) return 0.0f;

    VECTOR forward = _camera->GetGroundForward();
    return atan2f(forward.x, forward.z) * 180.0f / DX_PI_F;
}

bool TutorialDirector::IsReadGoal(TutorialGoal goal) {
    return goal == TutorialGoal::Read || goal == TutorialGoal::OwnHeat || goal == TutorialGoal::Finish;
}
