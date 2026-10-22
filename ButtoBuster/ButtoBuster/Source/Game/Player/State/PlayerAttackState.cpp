#include "PlayerAttackState.h"
#include "Player.h"
#include "PlayerActions.h"
#include "PlayerAttacks.h"
#include "PlayerIdleState.h"
#include "PlayerGuardState.h"
#include "PlayerJumpState.h"
#include "CombatSystem.h"
#include "BlowChain.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include <memory>

using std::make_unique;

namespace {
    // 振るときの声 溜めてから振ったときと対空斬り コンボの段ごとの声は PlayerAttacks の SLASH_VOICES
    constexpr const char* CHARGE_VOICE = "Player/Vc_Charge_Attack";
    constexpr const char* ANTI_AIR_VOICE = "Player/Vc_Attack2";

    // 振ったときの見た目 刃の軌跡 弧 足元の衝撃波
    struct SwingLook {
        // 刃の軌跡を判定の何フレーム前から、何フレーム後まで出すか
        // 判定より少しだけ長く出すと、振りの頭と終わりが自然に見える
        float trailLead = 1.0f;
        float trailTail = 1.5f;

        // 弧を判定の何フレーム前に出すか 少し前に出すと、刃の動きと弧が重なって見える
        float arcLead = 0.5f;

        // 弧の開きの上限 度 開きすぎると背中側まで回り込んで、どこを斬ったのか分からなくなる
        float arcMaxDegree = 220.0f;

        // 弧の半径と太さ 技の届く距離に掛ける割合
        float arcRadiusRate = 0.9f;
        float arcWidthRate = 0.4f;

        // 弧が出ている秒数 衝撃波も出す重い技は長く残す
        float arcLife = 0.2f;
        float heavyArcLife = 0.3f;

        // 衝撃波を広げる所 体の前へ、技の届く距離に掛ける割合だけ出た所
        float shockwaveForwardRate = 0.4f;
    };
    constexpr SwingLook SWING_LOOK{};

    // 当てたときに画面を揺らす長さ 秒 強さは技の shake
    constexpr float HIT_SHAKE_TIME = 0.2f;
}

PlayerAttackState::PlayerAttackState(const AttackData& data, int comboIndex, Kind kind, int chargeLevel)
    : _data(data)
    , _comboIndex(comboIndex)
    , _kind(kind)
    , _chargeLevel(chargeLevel) {
}

void PlayerAttackState::Enter(Player& player) {
    const PlayerData& playerData = player.data;

    // 強化で上がった倍率と、同じ技を続けた分の OP 相殺を掛ける ジャスト回避のあとなら、さらにこの振りが反撃になる
    // 技は GetName で見分けるので、段や溜めの段階が違えば別の技として数える
    // 反撃は締めまでのコンボ全体 締めの前の斬りはのけぞらせたまま重くし、締めの一振りで吹き飛ばして使い切る
    // 段のない技 (強斬り 溜め 対空斬り) は、それだけで締めになる
    _data = PlayerAttacks::ApplyRates(_data, playerData, player.UseMove(GetName()));
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
    // 倒した向きの近くの敵へ少しだけ吸い付ける
    if (_isFirstFrame) {
        _isFirstFrame = false;
        player.FaceImmediately(player.FindAimDirection(input.move, player.data.aimRadius));
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

    // 刃の軌跡は判定より少しだけ長く出す
    bool isTrailTime = time >= _data.hitStart - SWING_LOOK.trailLead && time <= _data.hitEnd + SWING_LOOK.trailTail;
    player.SetTrailEmitting(isTrailTime);

    PlaySwingEffects(player, time);
    ApplyHit(player);

    _queued.Remember(input);

    // あと隙の間は、回避もガードも次の技も出せない 押された技は覚えておき、明けたら出す
    if (!UpdateRecovery(player, deltaTime)) return;

    // あと隙が明けたら、押してあった技を先に出し、なければガードを見る
    if (TryContinue(player, input)) return;
    if (TryGuard(player, input)) return;

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
    if (_hasPlayedSwing || time < _data.hitStart - SWING_LOOK.arcLead) return;
    _hasPlayedSwing = true;

    PlayVoice();

    auto* effects = EffectManager::Get();
    if (!effects) return;

    bool hasShockwave = _data.shockwaveRadius > 0.0f;

    if (_data.hasArc) {
        EffectManager::ArcDesc arc;
        arc.center = VAdd(player.GetPosition(), VGet(0.0f, player.bodyHeight * 0.5f, 0.0f));
        arc.forward = player.GetForward();
        arc.radius = _data.reach * SWING_LOOK.arcRadiusRate;
        arc.width = _data.reach * SWING_LOOK.arcWidthRate;

        // 当たり判定の扇は正面から左右に arcDegree ずつ 見た目もそれに合わせる
        arc.arcDegree = _data.arcDegree * 2.0f;
        if (arc.arcDegree > SWING_LOOK.arcMaxDegree) arc.arcDegree = SWING_LOOK.arcMaxDegree;

        arc.tiltDegree = _data.arcTilt;
        arc.swing = _data.arcSwing;
        arc.color = _data.arcColor;
        arc.life = hasShockwave ? SWING_LOOK.heavyArcLife : SWING_LOOK.arcLife;
        effects->PlaySlashArc(arc);
    }

    // 空中で振ったときは地面を叩いていないので出さない
    if (hasShockwave && player.IsGrounded()) {
        VECTOR front = VAdd(player.GetPosition(), VScale(player.GetForward(), _data.reach * SWING_LOOK.shockwaveForwardRate));
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
    if (_data.shake > 0.0f) effects->Shake(_data.shake, HIT_SHAKE_TIME);
    PlayerActions::PlayZoomFlash(_data.zoomPunch);
}

bool PlayerAttackState::UpdateRecovery(Player& player, float deltaTime) {
    // まだ振っている途中 最後の判定が消えるまでは、あと隙を数えない
    if (player.GetAnimationTime() <= GetLastHitEnd() && !player.IsAnimationFinished()) return false;

    // アニメが先に終わっても数え続けられるよう、経った時間をアニメのフレームに直して数える
    _recoveryCount += deltaTime * ANIMATION_FPS * _data.animationSpeed;
    return _recoveryCount >= _data.recovery;
}

float PlayerAttackState::GetOpeningTime(const Player& player) const {
    // 最後の判定が消えるまでの残りと、あと隙の残りを足す どちらもアニメのフレームで数える
    float swingLeft = GetLastHitEnd() - player.GetAnimationTime();
    if (swingLeft < 0.0f) swingLeft = 0.0f;

    float recoveryLeft = _data.recovery - _recoveryCount;
    if (recoveryLeft < 0.0f) recoveryLeft = 0.0f;

    return (swingLeft + recoveryLeft) / (ANIMATION_FPS * _data.animationSpeed);
}

float PlayerAttackState::GetLastHitEnd() const {
    return (_data.hitStart2 >= 0.0f) ? _data.hitEnd2 : _data.hitEnd;
}

bool PlayerAttackState::TryGuard(Player& player, const InputInfo& input) {
    // 構えられるのは地上だけ 空中の技は今までどおり着地まで待つ
    if (!input.isGuardHeld || !player.IsGrounded()) return false;

    // あと隙は最後の判定が消えてから数えるので、ここではもう攻撃の判定は出ていない
    // 剣の軌跡の後片付けは Exit が行う
    return player.GetStates().Transition(this, make_unique<PlayerGuardState>());
}

bool PlayerAttackState::TryContinue(Player& player, const InputInfo& input) {
    Technique queued = _queued.technique;
    if (queued == Technique::None) return false;

    // ここへ来るのはあと隙が明けてから 回避はすぐ出す 危ないときに逃げられるように
    // ほかの技は受け付ける時間から 受け付ける時間の無い技も、振り終われば押してあった技を出す 対空斬りから空中の斬りへつなげるため
    bool canDodge = queued == Technique::Dodge;
    bool canCancel = (_data.cancelTime > 0.0f && player.GetAnimationTime() >= _data.cancelTime)
        || player.IsAnimationFinished();
    if (!canDodge && !canCancel) return false;

    bool isAirborne = !player.IsGrounded();

    // 段を進めるのは同じ場所で振り続けたときだけ 着地したり宙に出たりしたら、そこの 1 段目から振り直す
    Kind nextKind = isAirborne ? Kind::Air : Kind::Ground;
    bool isNextSlash = queued == Technique::Slash
        && _kind == nextKind
        && _comboIndex >= 0
        && _comboIndex + 1 < PlayerAttacks::SLASH_COUNT;
    if (isNextSlash) {
        int next = _comboIndex + 1;
        const AttackData& data = isAirborne ? PlayerAttacks::GetAirSlash(next) : PlayerAttacks::GetSlash(next);
        return player.GetStates().Transition(this, make_unique<PlayerAttackState>(data, next, nextKind));
    }

    // 回避は宙でも、今までどおり地上と同じ入口から出す ほかの技は、宙なら空中の技になる
    if (queued == Technique::Dodge) return PlayerActions::TryStart(player, this, _queued.ToInput(input));
    return _queued.TryStart(player, this, input);
}

void PlayerAttackState::PlayVoice() const {
    const char* voice = "";
    if (_chargeLevel >= 0) voice = CHARGE_VOICE;
    else if (_kind == Kind::AntiAir) voice = ANTI_AIR_VOICE;
    else if (_comboIndex >= 0 && _comboIndex < PlayerAttacks::SLASH_COUNT) voice = PlayerAttacks::SLASH_VOICES[_comboIndex];

    if (voice[0] != '\0') SoundManager::Instance().PlaySE(voice, PlayerData::VOICE_VOLUME);
}
