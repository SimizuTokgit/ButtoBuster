#include "EnemyAI.h"
#include "Enemy.h"
#include "EnemyIdleState.h"
#include "EnemyMoveState.h"
#include "ArenaWall.h"
#include "StageBuilder.h"
#include "PhaseDirector.h"
#include "Difficulty.h"
#include "GameObject.h"
#include <cmath>

namespace {
    using Node = BehaviorNodePtr<EnemyAI>;

    // 木を短く書くための関数
    Node If(bool (EnemyAI::*check)() const) {
        return std::make_unique<BehaviorCondition<EnemyAI>>(check);
    }

    Node Do(BehaviorStatus (EnemyAI::*run)(float)) {
        return std::make_unique<BehaviorAction<EnemyAI>>(run);
    }

    // 「条件がそろったらやる」枝
    Node When(Node condition, Node action) {
        auto sequence = std::make_unique<BehaviorSequence<EnemyAI>>();
        sequence->Add(std::move(condition));
        sequence->Add(std::move(action));
        return sequence;
    }
}

EnemyAI::~EnemyAI() {
    ReleaseToken();
}

void EnemyAI::Start() {
    _enemy = GetComponent<Enemy>();

    // 出てきた全員が同時に殴りに来ないよう、最初の番の取り方をばらけさせる
    _cooldown = RandomRange(0.3f, 1.5f);

    // 賢さが 1 以上なら行動の木で動く
    if (_enemy && _enemy->GetData().intelligence > 0) {
        BuildTree();
        _nextTechnique = RollTechnique();
        _flankOffset = RandomRange(-FLANK_SPREAD, FLANK_SPREAD);
    }
}

void EnemyAI::Update(float deltaTime) {
    if (!_enemy) return;

    // 相手 (プレイヤー) に時間を遅くされている間は、考えるのも動くのもゆっくりにする ジャスト回避のあとの反撃の間
    const Character* target = _enemy->GetTarget();
    float timeScale = target ? target->GetOpponentTimeScale() : 1.0f;
    _enemy->SetTimeScale(timeScale);
    float scaledDeltaTime = deltaTime * timeScale;

    InputInfo input;
    if (isPassive) input = Stand();
    else input = _tree ? ThinkWithTree(scaledDeltaTime) : Think(scaledDeltaTime);
    _enemy->Execute(input, scaledDeltaTime);
}

InputInfo EnemyAI::Stand() {
    InputInfo input;

    // 攻撃しないので番は持たない
    ReleaseToken();
    _enemy->ConsumeAttackFinished();
    _enemy->SetThinking("");

    const Character* target = _enemy->GetTarget();
    if (_enemy->IsDead() || !target) return input;

    VECTOR toTarget = VSub(target->GetPosition(), _enemy->GetPosition());
    toTarget.y = 0.0f;
    input.look = toTarget;
    return input;
}

// ===== 今までの動き (賢さ 0 の敵: Bee と Golem) =====

InputInfo EnemyAI::Think(float deltaTime) {
    InputInfo input;

    if (_enemy->IsDead()) {
        ReleaseToken();
        return input;
    }

    const Character* target = _enemy->GetTarget();
    if (!target || target->IsDead()) {
        ReleaseToken();
        return input;
    }

    VECTOR toTarget = VSub(target->GetPosition(), _enemy->GetPosition());
    toTarget.y = 0.0f;
    float distance = VSize(toTarget);
    input.look = toTarget;

    if (_hasToken) {
        _tokenTimer += deltaTime;
        if (_enemy->ConsumeAttackFinished() || _tokenTimer > TOKEN_TIMEOUT) {
            ReleaseToken();
            const EnemyData& data = _enemy->GetData();
            _cooldown = RollCooldown();
        }
    }
    else {
        // 番を持たずに終わった攻撃は数えない
        _enemy->ConsumeAttackFinished();
    }

    _cooldown -= deltaTime;

    bool wantsToken = !_hasToken && _cooldown <= 0.0f && distance < ENGAGE_DISTANCE;
    if (wantsToken) {
        auto* director = PhaseDirector::Get();
        if (director && director->GetTokens().TryAcquire(_enemy)) {
            _hasToken = true;
            _tokenTimer = 0.0f;
            _plannedTechnique = ChooseTechnique(distance);
        }
    }

    if (_hasToken) return Engage(input, toTarget, distance);
    return Surround(input, toTarget, deltaTime);
}

InputInfo EnemyAI::Engage(InputInfo input, VECTOR toTarget, float distance) {
    if (distance > GetRange(_plannedTechnique)) {
        input.move = VScale(toTarget, 1.0f / distance);
        return input;
    }

    VECTOR targetPosition = VAdd(_enemy->GetPosition(), toTarget);
    if (_enemy->GetFacingDot(targetPosition) > FACE_DOT_TO_ATTACK) {
        input.technique = _plannedTechnique;
    }
    return input;
}

InputInfo EnemyAI::Surround(InputInfo input, VECTOR toTarget, float deltaTime) {
    // 最初は今いる方角から回り始める 反対側へ横切って群れがぶつからないように
    if (!_isOrbitReady) {
        _orbitAngle = atan2f(-toTarget.z, -toTarget.x);
        _orbitDirection = (GetRand(1) == 0) ? 1.0f : -1.0f;
        _orbitSwitchTimer = RandomRange(2.0f, 4.0f);
        _isOrbitReady = true;
    }

    // ときどき回る向きを変える ずっと同じ向きだと動きが読めてしまう
    _orbitSwitchTimer -= deltaTime;
    if (_orbitSwitchTimer <= 0.0f) {
        if (GetRand(2) == 0) _orbitDirection = -_orbitDirection;
        _orbitSwitchTimer = RandomRange(2.0f, 4.0f);
    }
    _orbitAngle += _orbitDirection * ORBIT_SPEED * deltaTime;

    float radius = _enemy->GetData().surroundRadius;
    VECTOR targetPosition = VAdd(_enemy->GetPosition(), toTarget);
    VECTOR slot = VAdd(targetPosition, VGet(cosf(_orbitAngle) * radius, 0.0f, sinf(_orbitAngle) * radius));

    // プレイヤーが壁際にいると待つ場所が壁の外に出るので、壁の手前に寄せる 壁に向かって足踏みしないように
    slot = ArenaWall::ClampInside(slot, _enemy->bodyRadius + SLOT_WALL_MARGIN);

    VECTOR toSlot = VSub(slot, _enemy->GetPosition());
    toSlot.y = 0.0f;
    float slotDistance = VSize(toSlot);
    if (slotDistance < SLOT_ARRIVE_DISTANCE) return input;

    // 遠ければ走って追いつき、近ければ歩いて回り込む
    float amount = (slotDistance > SLOT_RUN_DISTANCE) ? 1.0f : 0.5f;
    input.move = VScale(toSlot, amount / slotDistance);
    return input;
}

Technique EnemyAI::ChooseTechnique(float distance) const {
    const EnemyData& data = _enemy->GetData();

    // 離れていれば撃つことが多い 近づいて刺すこともある
    if (data.canShoot && distance > data.attackRange * 1.5f && GetRand(99) < 70) {
        return Technique::Shoot;
    }
    if (data.hasHeavy && GetRand(99) < static_cast<int>(data.heavyChance * 100.0f)) {
        return Technique::StrongSlash;
    }
    return Technique::Slash;
}

float EnemyAI::GetRange(Technique technique) const {
    const EnemyData& data = _enemy->GetData();

    if (technique == Technique::Shoot) return data.shootRange;

    // 踏みつけは周りに当たるので、真下まで入らなくてよい
    if (technique == Technique::StrongSlash && data.heavyAreaRadius > 0.0f) {
        return data.heavyAreaRadius * 0.6f;
    }
    return data.attackRange;
}

void EnemyAI::ReleaseToken() {
    if (!_hasToken) return;
    _hasToken = false;

    if (auto* director = PhaseDirector::Get()) {
        director->GetTokens().Release(_enemy);
    }
}

float EnemyAI::RandomRange(float min, float max) {
    return min + (max - min) * (GetRand(1000) / 1000.0f);
}

// ===== 行動の木 (賢さ 1 以上の敵: Goblin と RedGoblin) =====

// 木の形 上の枝ほど優先する 毎フレーム上から見直し、条件のそろった最初の枝を進める
// 賢さが足りない枝は、はじめから木に入れない
// 枝の並びがそのまま優先の順なので、入れ替えると動きが変わる 並びは「動けない → 身を守る → 攻める → 待つ」
//
//  Selector (根)                                                       入る賢さ
//  │  ----- 動けない -----
//  ├ 振っている途中なら待つ ............ IsBusy      → Hold             1 から
//  │  ----- 身を守る -----
//  ├ 壁際で危ないなら離れる ............ IsCornered  → LeaveWall        4 から
//  ├ 溜めを見たら下がる ................ SeesCharge  → BackOff          2 から
//  │  ----- 攻める -----
//  ├ 背中を見たら踏み込む .............. SeesBack    → RushIn           5
//  ├ 隙を見たら踏み込む ................ SeesOpening → RushIn           3 から
//  ├ 届くなら振る ...................... IsInReach   → 振り方の Selector 1 から
//  │    ├ 構えていたら大振り ........... SeesGuard       → SwingHeavy   2 から
//  │    ├ 壁を割れるなら大振り ......... IsTargetAtLimit → SwingHeavy   2 から
//  │    └ ほかは決めておいた技 ......................... SwingPlanned 1 から
//  ├ 番が取れたら近づく ................ TakeToken   → Approach         1 から
//  │  ----- 待つ -----
//  └ 回って待つ / 背中側へ回り込む ................. Orbit / Flank    1 から / 3 から
void EnemyAI::BuildTree() {
    // ----- 1. この敵の賢さを決める -----
    // 表 (EnemyData.cpp) の賢さに、選んでいる難易度の増減 (Difficulty.cpp の intelligenceShift) を足す
    // 練習相手 (followsDifficulty が false) は、決めた賢さのまま
    int baseIntelligence = _enemy->GetData().intelligence;
    _intelligence = followsDifficulty ? GameMode::AdjustIntelligence(baseIntelligence) : baseIntelligence;
    const int intelligence = _intelligence;

    // 賢さが need 以上のときだけ、枝を Selector に入れる
    auto addIf = [intelligence](BehaviorSelector<EnemyAI>& selector, int need, Node branch) {
        if (intelligence >= need) selector.Add(std::move(branch));
    };

    // ----- 2. 振り方を選ぶ Selector (届く距離に入ったときに、根の枝から使う) -----
    // 大振りはガードできないので、構えている相手と、とどめを刺せる相手には大振り
    auto swing = std::make_unique<BehaviorSelector<EnemyAI>>();
    addIf(*swing, 2, When(If(&EnemyAI::SeesGuard),       Do(&EnemyAI::SwingHeavy)));
    addIf(*swing, 2, When(If(&EnemyAI::IsTargetAtLimit), Do(&EnemyAI::SwingHeavy)));
    addIf(*swing, 1, Do(&EnemyAI::SwingPlanned));   // ほかは、振り終わりに決めておいた技 (RollTechnique)

    // ----- 3. 根の Selector 上の枝ほど優先する -----
    auto root = std::make_unique<BehaviorSelector<EnemyAI>>();

    // 動けない 振っている のけぞり 吹き飛びの間は、体が入力を聞かないので何もしない
    addIf(*root, 1, When(If(&EnemyAI::IsBusy),      Do(&EnemyAI::Hold)));

    // 身を守る
    addIf(*root, 4, When(If(&EnemyAI::IsCornered),  Do(&EnemyAI::LeaveWall)));  // バースト値が溜まって壁際なら、割られないよう離れる
    addIf(*root, 2, When(If(&EnemyAI::SeesCharge),  Do(&EnemyAI::BackOff)));    // こちらを向いて溜めていたら、届かない所まで下がる

    // 攻める
    addIf(*root, 5, When(If(&EnemyAI::SeesBack),    Do(&EnemyAI::RushIn)));     // 背中を向けたら、番がなくても走って踏み込む
    addIf(*root, 3, When(If(&EnemyAI::SeesOpening), Do(&EnemyAI::RushIn)));     // 隙 (あと隙 回避の終わり 叩きつけの着地) に踏み込む
    addIf(*root, 1, When(If(&EnemyAI::IsInReach),   std::move(swing)));         // 届く距離なら、番がなくても振る
    addIf(*root, 1, When(Do(&EnemyAI::TakeToken),   Do(&EnemyAI::Approach)));   // 番が取れたら近づく 届いたら上の枝で振る

    // 待つ ほかは周りを回る 賢さ 3 からは相手の背中側へ回り込む
    addIf(*root, 1, Do(intelligence >= 3 ? &EnemyAI::Flank : &EnemyAI::Orbit));

    _tree = std::move(root);
}

InputInfo EnemyAI::ThinkWithTree(float deltaTime) {
    _input = InputInfo();
    _enemy->SetThinking("");

    const Character* target = _enemy->GetTarget();
    if (_enemy->IsDead() || !target || target->IsDead()) {
        ReleaseToken();
        return _input;
    }

    Sense(deltaTime);
    UpdateAttackCycle(deltaTime);

    // 向きはいつも相手へ 葉が move と technique を書き込む
    _input.look = _toTarget;
    _tree->Tick(*this, deltaTime);
    return _input;
}

void EnemyAI::Sense(float deltaTime) {
    const Character* target = _enemy->GetTarget();
    _toTarget = VSub(target->GetPosition(), _enemy->GetPosition());
    _toTarget.y = 0.0f;
    _distance = VSize(_toTarget);

    // 続いている間だけ数え、途切れたら 0 に戻す reactionTime を越えたら「気づいた」
    _guardTime = target->IsGuarding() ? _guardTime + deltaTime : 0.0f;
    _chargeTime = target->IsCharging() ? _chargeTime + deltaTime : 0.0f;
    _openingTime = (target->GetOpeningTime() > 0.0f) ? _openingTime + deltaTime : 0.0f;

    // 壁までの距離 十分に離れたか、バースト値が下がったら、壁から離れるのをやめる
    VECTOR fromCenter = VSub(_enemy->GetPosition(), StageBuilder::GetArenaCenter());
    fromCenter.y = 0.0f;
    _wallDistance = StageBuilder::ARENA_RADIUS - VSize(fromCenter);
    if (_wallDistance >= WALL_SAFE_DISTANCE || _enemy->GetBlowRatio() < SELF_DANGER_RATIO) _isLeavingWall = false;
}

void EnemyAI::UpdateAttackCycle(float deltaTime) {
    // 振り終わったら番を返し、次に振るまで待つ 次に振る技もここで決めておく
    if (_enemy->ConsumeAttackFinished()) {
        ReleaseToken();
        const EnemyData& data = _enemy->GetData();
        _cooldown = RollCooldown();
        _nextTechnique = RollTechnique();
    }
    _cooldown -= deltaTime;

    // 番を持ったまま振れずにいたら返す
    if (_hasToken) {
        _tokenTimer += deltaTime;
        if (_tokenTimer > TOKEN_TIMEOUT) ReleaseToken();
    }
}

float EnemyAI::RollCooldown() const {
    const EnemyData& data = _enemy->GetData();
    float rate = followsDifficulty ? GameMode::GetData().enemyCooldownRate : 1.0f;
    return RandomRange(data.cooldownMin, data.cooldownMax) * rate;
}

bool EnemyAI::Notices(float seenTime) const {
    // 賢いほど早く気づく 賢さの範囲の外は、そのままの速さ
    int count = static_cast<int>(sizeof(REACTION_RATES) / sizeof(REACTION_RATES[0]));
    float rate = (_intelligence >= 0 && _intelligence < count) ? REACTION_RATES[_intelligence] : 1.0f;
    return seenTime >= _enemy->GetData().reactionTime * rate;
}

Technique EnemyAI::RollTechnique() const {
    // ときどき大振り 毎フレーム引くと偏るので、振り終わりに 1 回だけ決める
    const EnemyData& data = _enemy->GetData();

    // 賢さ 1 は大振りを振らない 読みやすい斬りだけで来る
    if (_intelligence <= 1) return Technique::Slash;

    bool isHeavy = data.hasHeavy && GetRand(99) < static_cast<int>(data.heavyChance * 100.0f);
    return isHeavy ? Technique::StrongSlash : Technique::Slash;
}

// ----- 条件の葉 -----

bool EnemyAI::IsBusy() const {
    // Idle と Move のとき以外 (振っている のけぞっている 吹き飛んでいる) は、体が入力を聞かない
    auto& states = _enemy->GetStates();
    return !states.IsIn<EnemyIdleState>() && !states.IsIn<EnemyMoveState>();
}

bool EnemyAI::IsInReach() const {
    return _cooldown <= 0.0f && _distance <= _enemy->GetData().attackRange;
}

bool EnemyAI::SeesGuard() const {
    return Notices(_guardTime);
}

bool EnemyAI::IsTargetAtLimit() const {
    return _enemy->GetTarget()->GetBlowRatio() >= HEAVY_LIMIT_RATIO;
}

bool EnemyAI::SeesCharge() const {
    // こちらを向いて溜めている相手が近くにいるとき 背中側にいる敵は下がらない
    // 下がる先 (RETREAT_DISTANCE) より広く見ておく 下がりきった所で、また近づいて下がるのを繰り返さないように
    bool isFacingMe = _enemy->GetTarget()->GetFacingDot(_enemy->GetPosition()) > CHARGE_FACE_DOT;
    return Notices(_chargeTime) && isFacingMe && _distance < CHARGE_WATCH_DISTANCE;
}

bool EnemyAI::SeesOpening() const {
    return _cooldown <= 0.0f && Notices(_openingTime) && _distance < RUSH_DISTANCE;
}

bool EnemyAI::IsCornered() const {
    if (_enemy->GetBlowRatio() < SELF_DANGER_RATIO) return false;
    return _wallDistance < WALL_DANGER_DISTANCE || (_isLeavingWall && _wallDistance < WALL_SAFE_DISTANCE);
}

bool EnemyAI::SeesBack() const {
    // 相手の正面がこちらを向いていない (背中かほぼ横を向けている) とき
    bool isBackTurned = _enemy->GetTarget()->GetFacingDot(_enemy->GetPosition()) < BACK_TURNED_DOT;
    return _cooldown <= 0.0f && isBackTurned && _distance < BACKSTAB_DISTANCE;
}

// ----- 行動の葉 -----

BehaviorStatus EnemyAI::Hold(float deltaTime) {
    // 体の状態 (Attack Damage Blow) が F11 に出るので、頭の表示は空のまま
    return BehaviorStatus::Running;
}

BehaviorStatus EnemyAI::TakeToken(float deltaTime) {
    // 番を取るのは値を変えるので、条件ではなく行動の葉にしてある
    if (_hasToken) return BehaviorStatus::Success;
    if (_cooldown > 0.0f || _distance > ENGAGE_DISTANCE) return BehaviorStatus::Failure;

    auto* director = PhaseDirector::Get();
    if (!director || !director->GetTokens().TryAcquire(_enemy)) return BehaviorStatus::Failure;

    _hasToken = true;
    _tokenTimer = 0.0f;
    return BehaviorStatus::Success;
}

BehaviorStatus EnemyAI::Approach(float deltaTime) {
    _enemy->SetThinking("番を取って近づく");

    // 走って詰める 届いたら上の「届く距離で振る」枝に切り替わる
    if (_distance > 1.0f) _input.move = VScale(_toTarget, 1.0f / _distance);
    return BehaviorStatus::Running;
}

BehaviorStatus EnemyAI::SwingPlanned(float deltaTime) {
    bool isHeavy = _nextTechnique == Technique::StrongSlash;
    _enemy->SetThinking(isHeavy ? "大振り (ときどき)" : "斬る");
    return SwingAt(_nextTechnique);
}

BehaviorStatus EnemyAI::SwingHeavy(float deltaTime) {
    _enemy->SetThinking("大振り (ガード中かバースト値が溜まった)");
    return SwingAt(Technique::StrongSlash);
}

BehaviorStatus EnemyAI::BackOff(float deltaTime) {
    _enemy->SetThinking("溜めを見て下がる");

    // 下がっている間は殴りに行かないので、番は返す
    ReleaseToken();

    // 届かない所まで下がったら、その場で相手を見て待つ
    if (_distance > 1.0f && _distance < RETREAT_DISTANCE) {
        _input.move = VScale(_toTarget, -BACK_OFF_AMOUNT / _distance);
    }
    return BehaviorStatus::Running;
}

BehaviorStatus EnemyAI::RushIn(float deltaTime) {
    _enemy->SetThinking("隙に踏み込む");

    // 届く所まで走って詰め、届いたら速い斬りで突く
    if (_distance > _enemy->GetData().attackRange) {
        _input.move = VScale(_toTarget, 1.0f / _distance);
        return BehaviorStatus::Running;
    }
    return SwingAt(Technique::Slash);
}

BehaviorStatus EnemyAI::Orbit(float deltaTime) {
    _enemy->SetThinking("回って待つ");
    _input = Surround(_input, _toTarget, deltaTime);
    return BehaviorStatus::Running;
}

BehaviorStatus EnemyAI::Flank(float deltaTime) {
    _enemy->SetThinking("背中側へ回り込む");

    // 相手の背中側の角度へ、回る向きを合わせる 個体ごとに少しずらして重ならないようにする
    VECTOR back = VScale(_enemy->GetTarget()->GetForward(), -1.0f);
    float diff = atan2f(back.z, back.x) + _flankOffset - _orbitAngle;
    while (diff > DX_PI_F) diff -= DX_TWO_PI_F;
    while (diff < -DX_PI_F) diff += DX_TWO_PI_F;

    // 着いたら回らずに待つ 向かう間は普段より速く回る
    if (fabsf(diff) < FLANK_ARRIVE_ANGLE) {
        _orbitDirection = 0.0f;
    }
    else {
        _orbitDirection = (diff > 0.0f) ? FLANK_SPEED_RATE : -FLANK_SPEED_RATE;
    }

    _input = Surround(_input, _toTarget, deltaTime);
    return BehaviorStatus::Running;
}

BehaviorStatus EnemyAI::LeaveWall(float deltaTime) {
    _enemy->SetThinking("壁から離れる (バースト値が溜まった)");

    // 離れている間は殴りに行かないので、番は返す
    ReleaseToken();
    _isLeavingWall = true;

    // 真ん中のほうへ走る 向きは look で相手へ向けたまま
    VECTOR toCenter = VSub(StageBuilder::GetArenaCenter(), _enemy->GetPosition());
    toCenter.y = 0.0f;
    float length = VSize(toCenter);
    if (length > 1.0f) _input.move = VScale(toCenter, 1.0f / length);
    return BehaviorStatus::Running;
}

BehaviorStatus EnemyAI::SwingAt(Technique technique) {
    // 相手のほうを向いてから振る 向きは look で毎フレーム相手へ向けてある
    VECTOR targetPosition = VAdd(_enemy->GetPosition(), _toTarget);
    if (_enemy->GetFacingDot(targetPosition) > FACE_DOT_TO_ATTACK) _input.technique = technique;
    return BehaviorStatus::Running;
}
