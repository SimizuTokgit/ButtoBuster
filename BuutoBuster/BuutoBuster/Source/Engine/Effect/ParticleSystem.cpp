#include "ParticleSystem.h"
#include "ParticleManager.h"
#include "Transform.h"
#include <cmath>

ParticleSystem::~ParticleSystem() {
    ParticleManager::Instance().Unregister(this);
}

void ParticleSystem::Play() {
    _playing = true;
    _elapsedTime = 0.0f;
    _emissionAccumulator = 0.0f;
    ParticleManager::Instance().Register(this);
}

void ParticleSystem::Stop() {
    _playing = false;
}

bool ParticleSystem::IsPlaying() const {
    return _playing;
}

bool ParticleSystem::IsAlive() const {
    if (_playing) return true;
    for (const auto& p : _particles) {
        if (p.alive) return true;
    }
    return false;
}

void ParticleSystem::Emit(int count) {
    if (!_playing) {
        _playing = true;
        _elapsedTime = 0.0f;
        _emissionAccumulator = 0.0f;
        ParticleManager::Instance().Register(this);
    }
    for (int i = 0; i < count; ++i) {
        EmitSingle();
    }
}

void ParticleSystem::EmitSingle() {
    // 無効なパーティクルスロットを探す
    Particle* target = nullptr;
    for (auto& p : _particles) {
        if (!p.alive) {
            target = &p;
            break;
        }
    }

    // スロットがなければ新規追加（最大数以内）
    if (!target) {
        if (static_cast<int>(_particles.size()) >= maxParticles) return;
        _particles.emplace_back();
        target = &_particles.back();
    }

    // 放出位置を決定
    VECTOR basePos = transform ? transform->position : VGet(0, 0, 0);

    if (emissionShape == EmissionShape::Sphere && emissionRadius > 0.0f) {
        VECTOR offset = RandomPointInSphere(emissionRadius);
        target->position = VAdd(basePos, offset);
    } else {
        target->position = basePos;
    }

    // 速度を決定（ランダム方向 × ランダム速度）
    VECTOR dir = RandomEmitDirection();
    float speed = RandomFloat(startSpeedMin, startSpeedMax);
    target->velocity = VScale(dir, speed);

    // ライフタイム
    target->startLifetime = RandomFloat(startLifetimeMin, startLifetimeMax);
    target->remainingLifetime = target->startLifetime;

    // サイズ
    target->startSize = RandomFloat(startSizeMin, startSizeMax);

    // 不透明度
    target->alpha = 1.0f;

    // 有効化
    target->alive = true;
}

void ParticleSystem::Update(float deltaTime) {
    // エミッション処理
    if (_playing) {
        _elapsedTime += deltaTime;

        // 連続放出
        if (emissionRate > 0.0f) {
            _emissionAccumulator += emissionRate * deltaTime;
            while (_emissionAccumulator >= 1.0f) {
                EmitSingle();
                _emissionAccumulator -= 1.0f;
            }
        }

        // duration経過で放出停止
        if (!looping && _elapsedTime >= duration) {
            _playing = false;
        }
    }

    // パーティクル更新
    bool hasAlive = false;
    for (auto& p : _particles) {
        if (!p.alive) continue;

        // 減速（drag）
        if (drag > 0.0f) {
            float speed = VSize(p.velocity);
            if (speed > 0.0f) {
                float newSpeed = speed - drag * deltaTime;
                if (newSpeed <= 0.0f) {
                    p.velocity = VGet(0, 0, 0);
                } else {
                    p.velocity = VScale(VNorm(p.velocity), newSpeed);
                }
            }
        }

        // 重力
        p.velocity = VAdd(p.velocity, VScale(gravity, deltaTime));

        // 移動
        p.position = VAdd(p.position, VScale(p.velocity, deltaTime));

        // ライフタイム消費
        p.remainingLifetime -= deltaTime;
        if (p.remainingLifetime <= 0.0f) {
            p.alive = false;
            continue;
        }

        // アルファ（alphaFadeRatioに基づくフェード）
        float lifeRatio = p.remainingLifetime / p.startLifetime; // 1.0→0.0
        if (alphaFadeRatio > 0.0f && lifeRatio > (1.0f - alphaFadeRatio)) {
            // フェード開始前：アルファは1.0のまま
            p.alpha = 1.0f;
        } else if (alphaFadeRatio > 0.0f) {
            // フェード区間：残りライフタイム比率を0.0〜1.0にリマップ
            p.alpha = lifeRatio / (1.0f - alphaFadeRatio);
        } else {
            // alphaFadeRatio=0: 最初から線形にフェード
            p.alpha = lifeRatio;
        }

        hasAlive = true;
    }

    // 放出停止かつ全パーティクル消滅 → マネージャーから自動解除
    if (!_playing && !hasAlive) {
        ParticleManager::Instance().Unregister(this);
    }
}

void ParticleSystem::Render() {
    if (graphHandle < 0) return;

    // Zバッファを使用する（読み取りのみ、書き込みはしない）
    SetUseZBufferFlag(TRUE);
    SetWriteZBufferFlag(FALSE);

    // 描画色をセット
    SetDrawBright(color.r, color.g, color.b);

    if (stretch > 0.0f) {
        RenderStretched();
    } else if (useSubPass) {
        // 二重パス描画（旧Damageエフェクト互換）
        // Pass 1: 減算ブレンド
        for (const auto& p : _particles) {
            if (!p.alive) continue;
            float size = p.startSize * p.alpha;
            int alpha255 = static_cast<int>(p.alpha * 255.0f);
            SetDrawBlendMode(DX_BLENDMODE_SUB, alpha255);
            DrawBillboard3D(p.position, 0.5f, 0.5f, size, 0.0f, graphHandle, TRUE);
        }

        // Pass 2: 加算ブレンド
        for (const auto& p : _particles) {
            if (!p.alive) continue;
            float size = p.startSize * p.alpha;
            int alpha255 = static_cast<int>(p.alpha * 255.0f);
            SetDrawBlendMode(DX_BLENDMODE_ADD_X4, alpha255);
            DrawBillboard3D(p.position, 0.5f, 0.5f, size, 0.0f, graphHandle, TRUE);
        }
    } else {
        // 単一パス描画
        for (const auto& p : _particles) {
            if (!p.alive) continue;
            float size = p.startSize * p.alpha;
            int alpha255 = static_cast<int>(p.alpha * 255.0f);
            SetDrawBlendMode(blendMode, alpha255);
            DrawBillboard3D(p.position, 0.5f, 0.5f, size, 0.0f, graphHandle, TRUE);
        }
    }

    // 描画設定を元に戻す
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    SetDrawBright(255, 255, 255);
    SetUseZBufferFlag(FALSE);
}

float ParticleSystem::RandomFloat(float minVal, float maxVal) {
    float t = static_cast<float>(GetRand(10000)) / 10000.0f;
    return minVal + (maxVal - minVal) * t;
}

VECTOR ParticleSystem::RandomDirection() {
    VECTOR dir;
    float lenSq;
    do {
        dir.x = RandomFloat(-1.0f, 1.0f);
        dir.y = RandomFloat(-1.0f, 1.0f);
        dir.z = RandomFloat(-1.0f, 1.0f);
        lenSq = VSquareSize(dir);
    } while (lenSq < 0.0001f || lenSq > 1.0f);
    return VNorm(dir);
}

VECTOR ParticleSystem::RandomEmitDirection() const {
    if (spreadDegree >= 180.0f) return RandomDirection();

    float axisLength = VSize(emitDirection);
    if (axisLength < 0.0001f) return RandomDirection();
    VECTOR axis = VScale(emitDirection, 1.0f / axisLength);

    // 軸にでたらめな向きを混ぜてずらす 広がりが大きいほど多く混ぜる
    // 厳密に円錐の中で均等ではないが、火花の見た目には十分
    float spread = spreadDegree * DX_PI_F / 180.0f;
    if (spread > DX_PI_F * 0.5f) spread = DX_PI_F * 0.5f;
    VECTOR mixed = VAdd(VScale(axis, cosf(spread)), VScale(RandomDirection(), sinf(spread)));
    if (VSquareSize(mixed) < 0.0001f) return axis;
    return VNorm(mixed);
}

void ParticleSystem::RenderStretched() {
    // カメラから見て板が細くならないよう、進む向きと視線の両方に直交する向きへ広げる
    VECTOR cameraPosition = GetCameraPosition();

    _vertices.clear();
    auto makeVertex = [this](VECTOR position, float u, float v, int alpha) {
        VERTEX3D vertex{};
        vertex.pos = position;
        vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        vertex.dif = GetColorU8(color.r, color.g, color.b, alpha);
        vertex.spc = GetColorU8(0, 0, 0, 0);
        vertex.u = u;
        vertex.v = v;
        return vertex;
    };

    for (const auto& p : _particles) {
        if (!p.alive) continue;

        float speed = VSize(p.velocity);
        float size = p.startSize * p.alpha;
        float length = speed * stretch;
        if (length < size) length = size;

        VECTOR along = (speed > 0.001f) ? VScale(p.velocity, 1.0f / speed) : VGet(0.0f, 1.0f, 0.0f);
        VECTOR toCamera = VSub(cameraPosition, p.position);
        VECTOR side = VCross(along, toCamera);
        if (VSquareSize(side) < 0.0001f) continue;
        side = VScale(VNorm(side), size * 0.5f);

        VECTOR head = VAdd(p.position, VScale(along, size * 0.5f));
        VECTOR tail = VSub(p.position, VScale(along, length));
        int alpha255 = static_cast<int>(p.alpha * 255.0f);

        VERTEX3D headLeft = makeVertex(VAdd(head, side), 0.0f, 0.0f, alpha255);
        VERTEX3D headRight = makeVertex(VSub(head, side), 1.0f, 0.0f, alpha255);
        VERTEX3D tailLeft = makeVertex(VAdd(tail, side), 0.0f, 1.0f, alpha255);
        VERTEX3D tailRight = makeVertex(VSub(tail, side), 1.0f, 1.0f, alpha255);

        _vertices.push_back(headLeft);
        _vertices.push_back(headRight);
        _vertices.push_back(tailLeft);
        _vertices.push_back(tailLeft);
        _vertices.push_back(headRight);
        _vertices.push_back(tailRight);
    }

    if (_vertices.empty()) return;

    // 色と濃さは頂点に持たせたので、全体の明るさで二重に掛からないよう戻しておく
    SetDrawBright(255, 255, 255);
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(blendMode, 255);
    DrawPolygon3D(_vertices.data(), static_cast<int>(_vertices.size() / 3), graphHandle, TRUE);
    SetUseLighting(TRUE);
}

VECTOR ParticleSystem::RandomPointInSphere(float radius) {
    VECTOR dir = RandomDirection();
    float r = radius * std::cbrt(RandomFloat(0.0f, 1.0f));
    return VScale(dir, r);
}
