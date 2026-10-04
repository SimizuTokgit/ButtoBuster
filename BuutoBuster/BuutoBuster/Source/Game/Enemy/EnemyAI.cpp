#include "EnemyAI.h"
#include "Enemy.h"
#include "ArenaWall.h"
#include "PhaseDirector.h"
#include "GameObject.h"
#include <cmath>

EnemyAI::~EnemyAI() {
    ReleaseToken();
}

void EnemyAI::Start() {
    _enemy = GetComponent<Enemy>();

    // 出てきた全員が同時に殴りに来ないよう、最初の番の取り方をばらけさせる
    _cooldown = RandomRange(0.3f, 1.5f);
}

void EnemyAI::Update(float deltaTime) {
    if (!_enemy) return;

    InputInfo input = Think(deltaTime);
    _enemy->Execute(input, deltaTime);
}

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
            _cooldown = RandomRange(data.cooldownMin, data.cooldownMax);
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
