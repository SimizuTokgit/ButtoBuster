#pragma once
#include "MonoBehaviour.h"
#include "InputInfo.h"
#include "BehaviorTree.h"

class Enemy;
class Character;

// 敵の頭
// 状況を見て InputInfo を作り、Enemy に渡す
// PlayerController と同じ役割で、敵の体はどちらから来た入力かを知らない
//
// 賢さ (EnemyData の intelligence) が 1 以上の敵は、行動の木 (BehaviorTree.h) で動く 木の形は BuildTree
//   1: 番を取ったら近づいて振る 届く距離に入れば番がなくても振る ときどき大振り (heavyChance)
//   2: + ガード中とホカホカの相手には大振り、こちらを向いて溜めていたら下がる
//   3: + 相手の隙に踏み込む、待つ間は背中側へ回り込む
// 賢さ 0 の敵 (Bee と Golem) は今までどおり、番を持っていれば近づいて技を出し、持っていなければ相手の周りを回る
class EnemyAI : public MonoBehaviour {
private:
    // これより遠い相手には攻撃の番を取りにいかない
    static constexpr float ENGAGE_DISTANCE = 1100.0f;

    // 回り込む速さ ラジアン毎秒
    static constexpr float ORBIT_SPEED = 0.35f;

    // 番を持ったまま攻撃できずにいたら返す 斬られ続けて動けない敵が番を抱え込まないように
    static constexpr float TOKEN_TIMEOUT = 6.0f;

    // これくらい相手のほうを向いてから振る 背中を向けたまま振らないように
    static constexpr float FACE_DOT_TO_ATTACK = 0.85f;

    // 回り込み先にこれより近ければ、その場で相手を見て待つ
    static constexpr float SLOT_ARRIVE_DISTANCE = 30.0f;

    // 回り込み先がこれより遠ければ走って追いつく
    static constexpr float SLOT_RUN_DISTANCE = 500.0f;

    // 回り込み先が壁の外に出たら、体の太さとこの分だけ壁の手前に寄せる
    static constexpr float SLOT_WALL_MARGIN = 50.0f;

    // ----- 行動の木で使う数値 -----

    // 相手の吹っ飛ばされ値が、許容値に対してこの割合以上なら「許容値に届いた」とみなす (賢さ 2 から、とどめの大振りを振る)
    // 1 で、壁にぶつければ割れる状態 下げると、届く前から大振りで吹き飛ばしに来る
    static constexpr float HEAVY_LIMIT_RATIO = 1.0f;

    // 相手がこちらを向いているとみなす向き 1 で真正面 (賢さ 2 から、溜めを見て下がるときに使う)
    static constexpr float CHARGE_FACE_DOT = 0.5f;

    // この距離より近くでこちらを向いて溜めていたら、下がるか、止まって待つ
    static constexpr float CHARGE_WATCH_DISTANCE = 900.0f;

    // 溜めている相手から、この距離まで下がる 溜め斬りの届く距離より少し外
    static constexpr float RETREAT_DISTANCE = 500.0f;

    // 下がるときの倒し具合 0.6 を越えると走りになる
    static constexpr float BACK_OFF_AMOUNT = 0.55f;

    // 隙に踏み込むのは、この距離より近いとき (賢さ 3)
    static constexpr float RUSH_DISTANCE = 450.0f;

    // 回り込む先を個体ごとにずらす幅 ラジアン 0.8 で左右 45 度くらい (賢さ 3)
    static constexpr float FLANK_SPREAD = 0.8f;

    // 回り込む先との角度の差がこれより小さければ、着いたとみなして止まる ラジアン 背中の前後で揺れないように
    static constexpr float FLANK_ARRIVE_ANGLE = 0.15f;

    // 回り込むときは普段 (ORBIT_SPEED) の何倍の速さで回るか
    static constexpr float FLANK_SPEED_RATE = 2.0f;

    Enemy* _enemy = nullptr;

    bool _hasToken = false;
    float _tokenTimer = 0.0f;
    float _cooldown = 0.0f;
    Technique _plannedTechnique = Technique::None;

    bool _isOrbitReady = false;
    float _orbitAngle = 0.0f;
    float _orbitDirection = 1.0f;
    float _orbitSwitchTimer = 0.0f;

    // ----- 行動の木 (賢さ 1 以上の敵) -----
    BehaviorNodePtr<EnemyAI> _tree;
    InputInfo _input;                    // 葉が書き込む、このフレームの入力
    VECTOR _toTarget = VGet(0.0f, 0.0f, 0.0f);
    float _distance = 0.0f;
    float _guardTime = 0.0f;             // 相手がガードを続けている時間
    float _chargeTime = 0.0f;            // 相手が溜めを続けている時間
    float _openingTime = 0.0f;           // 相手が隙を見せ続けている時間
    Technique _nextTechnique = Technique::Slash;  // 次に振る技 振り終わりに決める
    float _flankOffset = 0.0f;           // 回り込む先を個体ごとにずらす角度

public:
    ~EnemyAI() override;

    void Start() override;
    void Update(float deltaTime) override;

    bool HasToken() const { return _hasToken; }

private:
    // ----- 今までの動き (賢さ 0) -----
    InputInfo Think(float deltaTime);
    InputInfo Engage(InputInfo input, VECTOR toTarget, float distance);
    InputInfo Surround(InputInfo input, VECTOR toTarget, float deltaTime);

    Technique ChooseTechnique(float distance) const;
    float GetRange(Technique technique) const;
    void ReleaseToken();

    static float RandomRange(float min, float max);

    // ----- 行動の木 -----
    void BuildTree();
    InputInfo ThinkWithTree(float deltaTime);

    // 見る 相手との距離と、ガード 溜め 隙が続いている時間を数える
    void Sense(float deltaTime);

    // 振り終わりの後始末 番を返し、次に振るまでの待ちを始め、次に振る技を決める
    void UpdateAttackCycle(float deltaTime);

    // 続いた時間が気づくまでの時間 (reactionTime) を越えたか
    bool Notices(float seenTime) const;

    Technique RollTechnique() const;

    // 条件の葉 値を変えずに見るだけ
    bool IsBusy() const;
    bool IsInReach() const;
    bool SeesGuard() const;
    bool IsTargetAtLimit() const;
    bool SeesCharge() const;
    bool SeesOpening() const;

    // 行動の葉 _input に書き込み、Running / Success / Failure を返す
    BehaviorStatus Hold(float deltaTime);
    BehaviorStatus TakeToken(float deltaTime);
    BehaviorStatus Approach(float deltaTime);
    BehaviorStatus SwingPlanned(float deltaTime);
    BehaviorStatus SwingHeavy(float deltaTime);
    BehaviorStatus BackOff(float deltaTime);
    BehaviorStatus RushIn(float deltaTime);
    BehaviorStatus Orbit(float deltaTime);
    BehaviorStatus Flank(float deltaTime);
    BehaviorStatus SwingAt(Technique technique);
};
