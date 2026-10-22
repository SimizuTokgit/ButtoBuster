#include "PhysicsManager.h"
#include "Collider.h"
#include "CapsuleCollider.h"
#include "MeshCollider.h"
#include "Rigidbody.h"
#include "Transform.h"
#include "GameObject.h"
#include "ContainerUtil.h"

using namespace std;

namespace {
    // ステージのポリゴンを集める範囲（キャラの位置からの半径）
    constexpr float SEARCH_DISTANCE = 500.0f;

    // 床判定の補正値（足元より少し下まで判定）
    constexpr float FLOOR_Y_ADJUST = -5.0f;

    // 床の高さを探すときの初期値 どの床よりも低くしておく
    constexpr float FLOOR_SEARCH_START_Y = -100000.0f;

    // 壁押し出し距離
    constexpr float HIT_SLIDE_DISTANCE = 1.0f;

    // 壁押し出しの最大試行回数
    constexpr int HIT_TRY_NUM = 16;
}

PhysicsManager& PhysicsManager::Instance() {
    static PhysicsManager instance;
    return instance;
}

void PhysicsManager::RegisterCollider(Collider* collider) {
    ContainerUtil::AddUnique(_colliders, collider);
}

void PhysicsManager::UnregisterCollider(Collider* collider) {
    ContainerUtil::Remove(_colliders, collider);

    // 衝突ペアの記録からも取り除く。
    // これを忘れると、破棄されたColliderが前フレームの衝突として残り、
    // 次のフレームのExit判定で解放済みポインタを触ってクラッシュする
    ForgetCollisionPairs(collider);
}

void PhysicsManager::ForgetCollisionPairs(Collider* collider) {
    if (!collider) return;

    auto purge = [collider](CollisionPairSet& pairs) {
        for (auto it = pairs.begin(); it != pairs.end(); ) {
            if (it->a == collider || it->b == collider) {
                it = pairs.erase(it);
            }
            else {
                ++it;
            }
        }
    };

    purge(_previousCollisions);
    purge(_currentCollisions);
    purge(_previousMeshCollisions);
    purge(_currentMeshCollisions);
}

void PhysicsManager::RegisterRigidbody(Rigidbody* rigidbody) {
    ContainerUtil::AddUnique(_rigidbodies, rigidbody);
}

void PhysicsManager::UnregisterRigidbody(Rigidbody* rigidbody) {
    ContainerUtil::Remove(_rigidbodies, rigidbody);
}

void PhysicsManager::RegisterMeshCollider(MeshCollider* meshCollider) {
    ContainerUtil::AddUnique(_meshColliders, meshCollider);
}

void PhysicsManager::UnregisterMeshCollider(MeshCollider* meshCollider) {
    ContainerUtil::Remove(_meshColliders, meshCollider);

    // MeshCollider も Collider なので、衝突ペアの記録から取り除く
    ForgetCollisionPairs(meshCollider);
}

void PhysicsManager::SetStageCollisionModel(int modelHandle) {
    // 後方互換用：直接モデルハンドルを使う場合
    // 新しい実装ではMeshColliderを使用することを推奨
    if (modelHandle != -1) {
        MV1SetupCollInfo(modelHandle, -1, 8, 8, 8);
    }
}

int PhysicsManager::GetStageCollisionModel() const {
    // 最初のMeshColliderのモデルハンドルを返す（後方互換用）
    if (!_meshColliders.empty() && _meshColliders[0]) {
        return _meshColliders[0]->ModelHandle;
    }
    return -1;
}

void PhysicsManager::Update(float deltaTime) {
    // 1. Rigidbodyの物理更新（重力、速度による移動）
    UpdateRigidbodies(deltaTime);

    // 2. ステージとの衝突判定・応答
    ProcessStageCollisions(deltaTime);

    // 3. Collider同士の衝突判定
    DetectCollisions();

    // 4. コールバック発火（Collider同士）
    InvokeCollisionCallbacks(_currentCollisions, _previousCollisions);

    // 5. MeshColliderとの衝突コールバック発火
    InvokeCollisionCallbacks(_currentMeshCollisions, _previousMeshCollisions);
}

bool PhysicsManager::CheckCapsuleStageHit(VECTOR pos1, VECTOR pos2, float radius) {
    // 全てのMeshColliderに対してチェック
    for (auto* meshCollider : _meshColliders) {
        if (!meshCollider || meshCollider->ModelHandle == -1) continue;
        if (!meshCollider->enabled) continue;

        MV1_COLL_RESULT_POLY_DIM hitDim = MV1CollCheck_Capsule(
            meshCollider->ModelHandle, -1, pos1, pos2, radius
        );

        bool hit = hitDim.HitNum > 0;
        MV1CollResultPolyDimTerminate(hitDim);

        if (hit) return true;
    }
    return false;
}

bool PhysicsManager::Linecast(VECTOR from, VECTOR to, VECTOR& outHitPosition) const {
    bool hasHit = false;
    float nearest = 0.0f;

    for (auto* meshCollider : _meshColliders) {
        if (!meshCollider || meshCollider->ModelHandle == -1) continue;
        if (!meshCollider->enabled) continue;

        MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(meshCollider->ModelHandle, -1, from, to);
        if (!hit.HitFlag) continue;

        // 複数に当たったら、線の始まりに一番近いものを採る
        float distance = VSize(VSub(hit.HitPosition, from));
        if (hasHit && distance >= nearest) continue;

        hasHit = true;
        nearest = distance;
        outHitPosition = hit.HitPosition;
    }
    return hasHit;
}

void PhysicsManager::UpdateRigidbodies(float deltaTime) {
    for (auto* rb : _rigidbodies) {
        if (rb && !rb->isKinematic) {
            rb->PhysicsUpdate(deltaTime * rb->timeScale);
        }
    }
}

void PhysicsManager::DetectCollisions() {
    _currentCollisions.clear();

    // 全Colliderペアをチェック
    for (size_t i = 0; i < _colliders.size(); ++i) {
        for (size_t j = i + 1; j < _colliders.size(); ++j) {
            auto* a = _colliders[i];
            auto* b = _colliders[j];

            if (!a || !b) continue;
            if (!a->enabled || !b->enabled) continue;
            if (a->gameObject == b->gameObject) continue;

            // カプセル同士の判定
            auto* capsuleA = dynamic_cast<CapsuleCollider*>(a);
            auto* capsuleB = dynamic_cast<CapsuleCollider*>(b);

            if (capsuleA && capsuleB) {
                Collision collision;
                if (CheckCapsuleCapsule(capsuleA, capsuleB, collision)) {
                    CollisionPair pair = { a, b };
                    _currentCollisions.insert(pair);

                    // Triggerでなければ押し出し処理
                    if (!a->isTrigger && !b->isTrigger) {
                        PushOutColliders(a, b, collision);
                    }
                }
            }
        }
    }
}

void PhysicsManager::ProcessStageCollisions(float deltaTime) {
    if (_meshColliders.empty()) return;

    // MeshCollider衝突ペアをクリア
    _currentMeshCollisions.clear();

    for (auto* rb : _rigidbodies) {
        if (!rb || rb->isKinematic || !rb->gameObject) continue;

        // CapsuleColliderを持つか確認
        auto* capsule = rb->gameObject->GetComponent<CapsuleCollider>();
        if (capsule && capsule->enabled) {
            ProcessCapsuleStageCollision(rb, capsule, deltaTime);
        }
    }
}

void PhysicsManager::ProcessCapsuleStageCollision(
    Rigidbody* rb,
    CapsuleCollider* capsule,
    float deltaTime
) {
    if (!rb || !capsule || !rb->transform) return;
    if (_meshColliders.empty()) return;

    VECTOR nextPos = rb->transform->localPosition;
    bool isGrounded = false;
    
    // 上向き速度がある場合はジャンプ中とみなす（床判定をしない）
    bool isJumping = rb->linearVelocity.y > 0.0f;

    // カプセルの座標を取得
    VECTOR capsulePos1, capsulePos2;
    capsule->GetCapsulePoints(capsulePos1, capsulePos2);

    // ポリゴンを種類別に分類（全MeshColliderから収集）
    struct PolyWithCollider {
        MV1_COLL_RESULT_POLY poly;
        MeshCollider* meshCollider;
    };
    vector<PolyWithCollider> floorPolys;
    vector<PolyWithCollider> wallPolys;
    vector<PolyWithCollider> ceilingPolys;

    // 衝突したMeshColliderを記録（コールバック用）
    vector<MeshCollider*> hitMeshColliders;

    // 全てのMeshColliderからポリゴンを収集
    for (auto* meshCollider : _meshColliders) {
        if (!meshCollider || meshCollider->ModelHandle == -1) continue;
        if (!meshCollider->enabled) continue;  // enabledがfalseはスキップ
        if (meshCollider->isTrigger) continue;  // Triggerは物理衝突しない

        int modelHandle = meshCollider->ModelHandle;

        // ステージとの衝突検出
        MV1_COLL_RESULT_POLY_DIM hitDim = MV1CollCheck_Sphere(
            modelHandle, -1, nextPos, SEARCH_DISTANCE
        );

        bool hitThisMesh = false;

        for (int i = 0; i < hitDim.HitNum; ++i) {
            MV1_COLL_RESULT_POLY poly = hitDim.Dim[i];
            VECTOR* normal = &poly.Normal;

            // 傾斜による分類
            float horizontalSlope = normal->x * normal->x + normal->z * normal->z;

            PolyWithCollider pwc = { poly, meshCollider };

            if (horizontalSlope > WALL_SLOPE_THRESHOLD) {
                // 壁ポリゴン
                wallPolys.push_back(pwc);
                hitThisMesh = true;
            }
            else if (normal->y <= 0.0f) {
                // 天井ポリゴン（下向き法線）
                if (rb->linearVelocity.y > 0.0f) {
                    ceilingPolys.push_back(pwc);
                    hitThisMesh = true;
                }
                else {
                    wallPolys.push_back(pwc);
                    hitThisMesh = true;
                }
            }
            else {
                // 床ポリゴン
                floorPolys.push_back(pwc);
                hitThisMesh = true;
            }
        }

        MV1CollResultPolyDimTerminate(hitDim);

        // このMeshColliderと衝突した場合、リストに追加
        if (hitThisMesh) {
            hitMeshColliders.push_back(meshCollider);
        }
    }

    // ポリゴンが見つからなければ接地していない
    if (floorPolys.empty() && wallPolys.empty() && ceilingPolys.empty()) {
		rb->isGrounded = false;
        return;
    }

    // === 壁との衝突処理 ===
    if (!wallPolys.empty()) {
        // 壁押し出し処理（複数回試行）
        for (int k = 0; k < HIT_TRY_NUM; k++) {
            bool hitWall = false;

            // カプセル座標を更新
            capsule->GetCapsulePoints(capsulePos1, capsulePos2);
            // 現在位置からのオフセットを計算
            VECTOR offset = VSub(nextPos, rb->transform->localPosition);
            capsulePos1 = VAdd(capsulePos1, offset);
            capsulePos2 = VAdd(capsulePos2, offset);

            for (auto& pwc : wallPolys) {
                if (HitCheck_Capsule_Triangle(
                    capsulePos1, capsulePos2, capsule->radius,
                    pwc.poly.Position[0], pwc.poly.Position[1], pwc.poly.Position[2]
                ) == TRUE) {
                    hitWall = true;

                    // 壁の法線方向に押し出す
                    VECTOR polyXZNormal = pwc.poly.Normal;
                    polyXZNormal.y = 0.0f;
                    float len = VSize(polyXZNormal);
                    if (len > 0.001f) {
                        polyXZNormal = VScale(polyXZNormal, 1.0f / len);
                        nextPos = VAdd(nextPos, VScale(polyXZNormal, HIT_SLIDE_DISTANCE));
                    }
                }
            }

            // どの壁にも当たらなくなったら終了
            if (!hitWall) {
                break;
            }
        }
    }

    // === 天井との衝突処理 ===
    if (!ceilingPolys.empty()) {
        // 天井判定用の線分座標を計算
        VECTOR lineTopPos = VAdd(nextPos, capsule->center);
        lineTopPos.y += capsule->height / 2.0f;

        VECTOR lineBottomPos = VAdd(nextPos, capsule->center);
        lineBottomPos.y -= capsule->height / 2.0f;

        float minY = lineTopPos.y;
        bool hitCeiling = false;

        for (auto& pwc : ceilingPolys) {
            // 線分とポリゴンの当たり判定
            HITRESULT_LINE lineRes = HitCheck_Line_Triangle(
                lineBottomPos, lineTopPos,
                pwc.poly.Position[0], pwc.poly.Position[1], pwc.poly.Position[2]
            );

            if (lineRes.HitFlag == TRUE) {
                if (lineRes.Position.y < minY) {
                    minY = lineRes.Position.y;
                    hitCeiling = true;
                }
            }
        }

        if (hitCeiling) {
            // 頭がぶつかったので位置を補正
            nextPos.y = minY - (lineTopPos.y - lineBottomPos.y);

            // Y軸方向の速度を0に
            if (rb->linearVelocity.y > 0.0f) {
                rb->linearVelocity.y = 0.0f;
            }
        }
    }

    // === 床との衝突処理（線分判定を使用） ===
    // ジャンプ中（上向き速度がある）場合は床判定をスキップ
    if (!floorPolys.empty() && !isJumping) {
        // 床判定用の線分座標を計算
        VECTOR lineTopPos = VAdd(nextPos, capsule->center);
        lineTopPos.y += capsule->height / 2.0f + capsule->radius;

        VECTOR lineBottomPos = VAdd(nextPos, capsule->center);
        lineBottomPos.y -= capsule->height / 2.0f + capsule->radius;

        // 足元より少し下まで判定（坂道で浮かないように）
        lineBottomPos.y += FLOOR_Y_ADJUST;

        float maxY = FLOOR_SEARCH_START_Y;
        bool hitFloor = false;

        for (auto& pwc : floorPolys) {
            // 線分とポリゴンの当たり判定
            HITRESULT_LINE lineRes = HitCheck_Line_Triangle(
                lineTopPos, lineBottomPos,
                pwc.poly.Position[0], pwc.poly.Position[1], pwc.poly.Position[2]
            );

            if (lineRes.HitFlag == TRUE) {
                if (lineRes.Position.y > maxY) {
                    maxY = lineRes.Position.y;
                    hitFloor = true;
                }
            }
        }

        if (hitFloor) {
            // 床に当たった
            isGrounded = true;

            // Y座標を床の高さに合わせる
            nextPos.y = maxY;

            // 落下中なら速度を0に
            if (rb->linearVelocity.y < 0.0f) {
                rb->linearVelocity.y = 0.0f;
            }
        }
    }

    // 位置を更新
    rb->transform->localPosition = nextPos;
    rb->isGrounded = isGrounded;

    // MeshColliderとの衝突ペアを記録（コールバック用）
    for (auto* meshCollider : hitMeshColliders) {
        CollisionPair pair = { capsule, meshCollider };
        _currentMeshCollisions.insert(pair);
    }
}

VECTOR PhysicsManager::CalculateSlideVector(VECTOR moveVector, VECTOR wallNormal) {
    // 壁の法線のXZ成分のみ使用
    VECTOR xzNormal = wallNormal;
    xzNormal.y = 0.0f;

    float len = VSize(xzNormal);
    if (len < 0.001f) {
        return moveVector;
    }
    xzNormal = VScale(xzNormal, 1.0f / len);

    // 移動ベクトルと壁法線の外積 → 壁に沿った方向
    VECTOR crossY = VCross(moveVector, xzNormal);
    VECTOR slideVec = VCross(xzNormal, crossY);

    // Y軸にはスライドしない
    slideVec.y = 0.0f;

    return slideVec;
}

bool PhysicsManager::CheckCapsuleCapsule(
    CapsuleCollider* a,
    CapsuleCollider* b,
    Collision& outCollision
) {
    if (!a || !b) return false;

    VECTOR a1, a2, b1, b2;
    a->GetCapsulePoints(a1, a2);
    b->GetCapsulePoints(b1, b2);

    // DxLibのカプセル同士判定
    if (HitCheck_Capsule_Capsule(a1, a2, a->radius, b1, b2, b->radius) == TRUE) {
        // 衝突情報を作成
        VECTOR centerA = a->GetWorldCenter();
        VECTOR centerB = b->GetWorldCenter();
        VECTOR dir = VSub(centerB, centerA);
        dir.y = 0.0f;

        float dist = VSize(dir);
        if (dist > 0.001f) {
            dir = VScale(dir, 1.0f / dist);
        }
        else {
            dir = VGet(1, 0, 0);
        }

        float totalRadius = a->radius + b->radius;
        float penetration = totalRadius - dist;
        if (penetration < 0.0f) penetration = 0.0f;

        outCollision = CreateCollision(
            b,
            VScale(VAdd(centerA, centerB), 0.5f),
            dir,
            penetration
        );

        return true;
    }

    return false;
}

void PhysicsManager::PushOutColliders(
    Collider* a,
    Collider* b,
    const Collision& collision
) {
    if (!a || !b) return;

    auto* rbA = a->gameObject ? a->gameObject->GetComponent<Rigidbody>() : nullptr;
    auto* rbB = b->gameObject ? b->gameObject->GetComponent<Rigidbody>() : nullptr;

    // 両方Kinematicまたは両方Rigidbodyなしは何もしない
    bool aMovable = rbA && !rbA->isKinematic;
    bool bMovable = rbB && !rbB->isKinematic;

    if (!aMovable && !bMovable) return;

    VECTOR pushDir = collision.contactNormal;
    float pushAmount = collision.penetrationDepth * 0.5f + PUSH_POWER;

    if (aMovable && bMovable) {
        // 両方動ける場合、半分ずつ押し出す
        VECTOR push = VScale(pushDir, pushAmount * 0.5f);
        a->transform->localPosition = VSub(a->transform->localPosition, push);
        b->transform->localPosition = VAdd(b->transform->localPosition, push);
    }
    else if (aMovable) {
        // Aだけ動ける
        VECTOR push = VScale(pushDir, -pushAmount);
        a->transform->localPosition = VAdd(a->transform->localPosition, push);
    }
    else if (bMovable) {
        // Bだけ動ける
        VECTOR push = VScale(pushDir, pushAmount);
        b->transform->localPosition = VAdd(b->transform->localPosition, push);
    }
}

void PhysicsManager::InvokeCollisionCallbacks(const CollisionPairSet& current, CollisionPairSet& previous) {
    // ペアの両側へ同じ種類のコールバックを送る
    // トリガーなら相手の Collider* を、物理衝突なら相手を入れた Collision を渡す
    auto invokeBoth = [this](
        const CollisionPair& pair,
        void (Collider::*onTrigger)(Collider*),
        void (Collider::*onCollision)(const Collision&)
    ) {
        if (pair.a->isTrigger || pair.b->isTrigger) {
            (pair.a->*onTrigger)(pair.b);
            (pair.b->*onTrigger)(pair.a);
        }
        else {
            Collision collisionA = CreateCollision(pair.b, VGet(0, 0, 0), VGet(0, 1, 0), 0);
            Collision collisionB = CreateCollision(pair.a, VGet(0, 0, 0), VGet(0, 1, 0), 0);
            (pair.a->*onCollision)(collisionA);
            (pair.b->*onCollision)(collisionB);
        }
    };

    // Enter: 今回衝突 && 前回非衝突
    // Stay: 今回衝突 && 前回衝突
    for (const auto& pair : current) {
        bool wasPrevious = previous.find(pair) != previous.end();

        if (!wasPrevious) {
            invokeBoth(pair, &Collider::InvokeOnTriggerEnter, &Collider::InvokeOnCollisionEnter);
        }
        else {
            invokeBoth(pair, &Collider::InvokeOnTriggerStay, &Collider::InvokeOnCollisionStay);
        }
    }

    // Exit: 今回非衝突 && 前回衝突
    for (const auto& pair : previous) {
        bool isCurrent = current.find(pair) != current.end();

        if (!isCurrent) {
            invokeBoth(pair, &Collider::InvokeOnTriggerExit, &Collider::InvokeOnCollisionExit);
        }
    }

    // 現在の衝突を前フレーム用に保存
    previous = current;
}

Collision PhysicsManager::CreateCollision(
    Collider* other,
    VECTOR contactPoint,
    VECTOR contactNormal,
    float penetration
) {
    Collision collision;
    collision.collider = other;
    collision.contactPoint = contactPoint;
    collision.contactNormal = contactNormal;
    collision.penetrationDepth = penetration;

    if (other) {
        collision.gameObject = other->gameObject;
        collision.transform = other->transform;
        if (other->gameObject) {
            collision.rigidbody = other->gameObject->GetComponent<Rigidbody>();
        }
    }

    return collision;
}
