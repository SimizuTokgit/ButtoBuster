#include "SlashTrail.h"
#include "SkinnedMeshRenderer.h"
#include "EffectDraw.h"
#include "Time.h"
#include <algorithm>

using namespace EffectDraw;

SlashTrail::~SlashTrail() {
    if (_graph != -1) DeleteGraph(_graph);
}

bool SlashTrail::Setup(SkinnedMeshRenderer* owner, const char* boneName,
    VECTOR baseLocal, VECTOR tipLocal, const char* texturePath) {
    _owner = owner;
    _boneIndex = owner ? owner->FindBone(boneName) : -1;
    _baseLocal = baseLocal;
    _tipLocal = tipLocal;
    _graph = LoadGraph(texturePath);

    renderQueue = RENDER_QUEUE_TRANSPARENT;
    Register();

    return _boneIndex != -1 && _graph != -1;
}

void SlashTrail::Render() {
    Record();
    Draw();
}

void SlashTrail::Record() {
    // ヒットストップ中は残像も止めたいので、倍率のかかった時間で古くする
    float deltaTime = Time::DeltaTime();
    for (auto& sample : _samples) {
        sample.age += deltaTime;
    }

    _samples.erase(
        std::remove_if(_samples.begin(), _samples.end(),
            [](const Sample& sample) { return sample.age > SAMPLE_LIFE; }),
        _samples.end());

    if (!_isEmitting || !_owner || _boneIndex == -1) return;

    MATRIX bone = _owner->GetBoneMatrix(_boneIndex);
    Sample sample;
    sample.base = VTransform(_baseLocal, bone);
    sample.tip = VTransform(_tipLocal, bone);
    sample.age = 0.0f;

    _samples.insert(_samples.begin(), sample);
    if (static_cast<int>(_samples.size()) > MAX_SAMPLES) _samples.pop_back();
}

void SlashTrail::Draw() {
    if (!enabled || _graph == -1 || _samples.size() < 2) return;

    _vertices.clear();

    float last = static_cast<float>(_samples.size() - 1);
    for (size_t i = 0; i + 1 < _samples.size(); ++i) {
        const Sample& a = _samples[i];
        const Sample& b = _samples[i + 1];

        // 新しいほど濃く 古いほど薄く
        float alphaA = 1.0f - a.age / SAMPLE_LIFE;
        float alphaB = 1.0f - b.age / SAMPLE_LIFE;
        float uA = i / last;
        float uB = (i + 1) / last;

        PushQuad(_vertices,
            MakeVertex(a.base, trailColor, alphaA, uA, 1.0f),
            MakeVertex(a.tip, trailColor, alphaA, uA, 0.0f),
            MakeVertex(b.base, trailColor, alphaB, uB, 1.0f),
            MakeVertex(b.tip, trailColor, alphaB, uB, 0.0f));
    }

    // 描き終えたら、2D の描画に響かないよう元の状態に戻る
    TransparentScope scope(DX_BLENDMODE_ADD);
    DrawPolygon3D(_vertices.data(), static_cast<int>(_vertices.size() / 3), _graph, TRUE);
}
