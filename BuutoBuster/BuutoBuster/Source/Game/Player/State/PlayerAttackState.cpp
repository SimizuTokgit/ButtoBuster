#include "PlayerAttackState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerAttacks.h"
#include "PlayerIdleState.h"
#include "PlayerJumpState.h"
#include "CombatSystem.h"
#include "BlowChain.h"
#include "EffectManager.h"
#include <memory>

using std::make_unique;

PlayerAttackState::PlayerAttackState(const AttackData& data, int comboIndex, Kind kind, int chargeLevel)
    : _data(data)
    , _comboIndex(comboIndex)
    , _kind(kind)
    , _chargeLevel(chargeLevel) {
}

void PlayerAttackState::Enter(Player& player) {
    const PlayerData& playerData = player.data;

    // 強化で上がった倍率を掛ける ジャスト回避のあとなら、さらにこの振りが反撃になる
    // 反撃は締めまでのコンボ全体 締めの前の斬りはのけぞらせたまま重くし、締めの一振りで吹き飛ばして使い切る
    // 段のない技 (強斬り 溜め 対空斬り) は、それだけで締めになる
    _data = PlayerAttacks::ApplyRates(_data, playerData);
    if (player.HasCounter()) {
        bool isFinisher = _comboIndex < 0 || _comboIndex >= PlayerAttacks::SLASH_COUNT - 1;
        if (!isFinisher) {
            _data = PlayerAttacks::CreateCounterCombo(_data, playerData);
        }
        else if (player.ConsumeCounter()) {
            _data = PlayerAttacks::CreateCounter(_data, playerData);
        }
    }

    bool isFromCharge = _chargeLevel >= 0;
    player.PlayAnimation(_data.animationName, _data.animationSpeed, !isFromCharge);
    player.SetTrailEmitting(false);

    _chain = BlowChain::Create(&player.GetChainEvents());

    if (player.IsGrounded()) {
        if (_kind == Kind::AntiAir) player.SetVerticalVelocity(playerData.antiAirJumpSpeed);
        return;
    }

    // 空中で振るときは少し浮き直し、振っている間はゆっくり落ちる
    // 浮き直せるのは着地までに決まった回数だけ いつまでも宙にいられないように
    // 跳び上がっている途中なら、その勢いは止めない
    _isHanging = player.TryUseAirHang();
    if (_isHanging && player.GetVelocity().y < playerData.airHangSpeed) {
        player.SetVerticalVelocity(playerData.airHangSpeed);
    }
}

void PlayerAttackState::Execute(Player& player, const InputInfo& input, float deltaTime) {
    // 向きは振り始めの1回だけ決める 振っている途中で回ると当たりがぶれる
    // ロックオン中は狙った相手へ振る そうでなければ、倒した向きの近くの敵へ少しだけ吸い付ける
    if (_isFirstFrame) {
        _isFirstFrame = false;
        bool isLockedOn = VSquareSize(input.look) > 0.0001f;
        player.FaceImmediately(isLockedOn ? input.look : player.FindAimDirection(input.move, player.data.aimRadius));
    }

    float time = player.GetAnimationTime();

    // 振り終わるまでは前に出る
    if (time < _data.hitEnd) {
        player.SetHorizontalVelocity(player.GetForward(), _data.lunge);
    }
    else {
        player.StopHorizontal();
    }

    // 浮き直した振りの間は、落ちる速さを抑えて宙に留まる
    float fallSpeed = player.data.airFallSpeed;
    if (_isHanging && !player.IsGrounded() && player.GetVelocity().y < -fallSpeed) {
        player.SetVerticalVelocity(-fallSpeed);
    }

    // 軌跡は判定より少しだけ長く出すと、振りの頭と終わりが自然に見える
    bool isTrailTime = time >= _data.hitStart - 1.0f && time <= _data.hitEnd + 1.5f;
    player.SetTrailEmitting(isTrailTime);

    PlaySwingEffects(player, time);
    ApplyHit(player);

    if (input.technique != Technique::None) _queued = input.technique;

    if (TryContinue(player, input)) return;

    if (!player.IsAnimationFinished()) return;

    // 対空斬りや空中の斬りは空中で振り終わるので、着地まで落ちるのを待つ
    if (!player.IsGrounded()) {
        player.GetStates().Transition(this, make_unique<PlayerJumpState>(false));
        return;
    }

    player.GetStates().Transition(this, make_unique<PlayerIdleState>());
}

void PlayerAttackState::Exit(Player& player) {
    player.SetTrailEmitting(false);
    player.StopHorizontal();
}

const char* PlayerAttackState::GetName() const {
    if (_kind == Kind::AntiAir) return "AntiAir";

    if (_chargeLevel >= 0) {
        static const char* heavyNames[] = { "Heavy", "Heavy1", "Heavy2", "Heavy3" };
        return heavyNames[_chargeLevel];
    }
    if (_comboIndex < 0) return "Strong";

    static const char* names[] = { "Slash1", "Slash2", "Slash3" };
    static const char* airNames[] = { "AirSlash1", "AirSlash2", "AirSlash3" };
    return (_kind == Kind::Air) ? airNames[_comboIndex] : names[_comboIndex];
}

void PlayerAttackState::PlaySwingEffects(Player& player, float time) {
    // 判定が出る少し前に出すと、刃の動きと弧が重なって見える
    constexpr float SWING_LEAD = 0.5f;

    // 開きすぎると背中側まで回り込んで、どこを斬ったのか分からなくなる
    constexpr float MAX_ARC_DEGREE = 220.0f;

    if (_hasPlayedSwing || time < _data.hitStart - SWING_LEAD) return;
    _hasPlayedSwing = true;

    auto* effects = EffectManager::Get();
    if (!effects) return;

    bool hasShockwave = _data.shockwaveRadius > 0.0f;

    if (_data.hasArc) {
        EffectManager::ArcDesc arc;
        arc.center = VAdd(player.GetPosition(), VGet(0.0f, player.bodyHeight * 0.5f, 0.0f));
        arc.forward = player.GetForward();
        arc.radius = _data.reach * 0.9f;
        arc.width = _data.reach * 0.4f;

        // 当たり判定の扇は正面から左右に arcDegree ずつ 見た目もそれに合わせる
        arc.arcDegree = _data.arcDegree * 2.0f;
        if (arc.arcDegree > MAX_ARC_DEGREE) arc.arcDegree = MAX_ARC_DEGREE;

        arc.tiltDegree = _data.arcTilt;
        arc.swing = _data.arcSwing;
        arc.color = _data.arcColor;
        arc.life = hasShockwave ? 0.3f : 0.2f;
        effects->PlaySlashArc(arc);
    }

    // 空中で振ったときは地面を叩いていないので出さない
    if (hasShockwave && player.IsGrounded()) {
        VECTOR front = VAdd(player.GetPosition(), VScale(player.GetForward(), _data.reach * 0.4f));
        effects->PlayShockwave(front, _data.shockwaveRadius, _data.arcColor);
    }
}

void PlayerAttackState::ApplyHit(Player& player) {
    float time = player.GetAnimationTime();

    bool isFirstWindow = time >= _data.hitStart && time <= _data.hitEnd;
    bool isSecondWindow = _data.hitStart2 >= 0.0f && time >= _data.hitStart2 && time <= _data.hitEnd2;

    // 2回目の判定に入ったら、1回目で当てた相手にもう一度当てられるようにする
    if (isSecondWindow && !_hasEnteredSecondHit) {
        _hasEnteredSecondHit = true;
        _hitList.clear();
    }

    if (!isFirstWindow && !isSecondWindow) return;

    bool isFirstHit = _hitList.empty();
    int hits = CombatSystem::ApplyMelee(player, _data, _hitList, _chain);
    if (hits <= 0) return;

    player.AddCombo(hits);

    // まとめて当たっても止めるのは1回だけ 何体も斬るたびに止まると重くなる
    if (!isFirstHit) return;

    auto* effects = EffectManager::Get();
    if (!effects) return;
    effects->HitStop(_data.hitStop);
    if (_data.shake > 0.0f) effects->Shake(_data.shake, 0.2f);

    // 寄る技ほど重いので、画面も強く光らせる
    if (_data.zoomPunch > 0.0f) {
        effects->ZoomPunch(_data.zoomPunch, 0.35f);
        effects->FlashScreen(0xFFFFFF, _data.zoomPunch * 0.04f, 0.12f);
    }
}

bool PlayerAttackState::TryContinue(Player& player, const InputInfo& input) {
    if (_queued == Technique::None) return false;

    float time = player.GetAnimationTime();

    // 回避だけは振り終わった直後から受け付ける 危ないときに逃げられるように
    // 受け付ける時間の無い技も、振り終われば押してあった技を出す 対空斬りから空中の斬りへつなげるため
    bool canDodge = _queued == Technique::Dodge && time > _data.hitEnd;
    bool canCancel = (_data.cancelTime > 0.0f && time >= _data.cancelTime) || player.IsAnimationFinished();
    if (!canDodge && !canCancel) return false;

    auto& states = player.GetStates();
    bool isAirborne = !player.IsGrounded();

    // 段を進めるのは同じ場所で振り続けたときだけ 着地したり宙に出たりしたら、そこの 1 段目から振り直す
    Kind nextKind = isAirborne ? Kind::Air : Kind::Ground;
    bool isNextSlash = _queued == Technique::Slash
        && _kind == nextKind
        && _comboIndex >= 0
        && _comboIndex + 1 < PlayerAttacks::SLASH_COUNT;
    if (isNextSlash) {
        int next = _comboIndex + 1;
        const AttackData& data = isAirborne ? PlayerAttacks::GetAirSlash(next) : PlayerAttacks::GetSlash(next);
        return states.Transition(this, make_unique<PlayerAttackState>(data, next, nextKind));
    }

    InputInfo buffered = input;
    buffered.technique = _queued;

    // 空中では空中の技を出す 回避は今までどおり地上と同じ入口から出す
    if (isAirborne && _queued != Technique::Dodge) {
        return PlayerActions::TryStartAir(player, this, buffered);
    }
    return PlayerActions::TryStart(player, this, buffered);
}
