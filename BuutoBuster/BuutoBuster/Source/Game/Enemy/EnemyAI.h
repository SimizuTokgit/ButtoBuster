#pragma once
#include "MonoBehaviour.h"
#include "InputInfo.h"

class Enemy;
class Character;

// 敵の頭
// 状況を見て InputInfo を作り、Enemy に渡す
// PlayerController と同じ役割で、敵の体はどちらから来た入力かを知らない
//
// 攻撃の番を持っていれば近づいて技を出し、持っていなければ相手の周りを回る
// 振る瞬間に相手がガードしているか、許容値に届いていれば、斬りを大振りに切り替える (EnemyData の heavyVsGuard / heavyVsLimit)
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

    // 相手の吹っ飛ばされ値が、許容値に対してこの割合以上なら「許容値に届いた」とみなす (heavyVsLimit の敵が大振りに切り替える)
    // 1 で、壁にぶつければ割れる状態 下げると、届く前から大振りで吹き飛ばしに来る
    static constexpr float HEAVY_LIMIT_RATIO = 1.0f;

    Enemy* _enemy = nullptr;

    bool _hasToken = false;
    float _tokenTimer = 0.0f;
    float _cooldown = 0.0f;
    Technique _plannedTechnique = Technique::None;

    bool _isOrbitReady = false;
    float _orbitAngle = 0.0f;
    float _orbitDirection = 1.0f;
    float _orbitSwitchTimer = 0.0f;

public:
    ~EnemyAI() override;

    void Start() override;
    void Update(float deltaTime) override;

    bool HasToken() const { return _hasToken; }

private:
    InputInfo Think(float deltaTime);
    InputInfo Engage(InputInfo input, VECTOR toTarget, float distance);
    InputInfo Surround(InputInfo input, VECTOR toTarget, float deltaTime);

    Technique ChooseTechnique(float distance) const;

    // 振る瞬間の相手を見て、斬りを大振りに切り替えるか
    bool ShouldSwitchToHeavy(const Character& target) const;

    float GetRange(Technique technique) const;
    void ReleaseToken();

    static float RandomRange(float min, float max);
};
