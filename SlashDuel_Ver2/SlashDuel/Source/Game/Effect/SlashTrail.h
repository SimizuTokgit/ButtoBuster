#pragma once
#include "Renderer.h"
#include "DxLib.h"
#include <vector>

class SkinnedMeshRenderer;

// 剣の軌跡
// 刃の根元と先の位置を記録していき、帯状の板でつなぐ
//
// 記録は描く直前に行う
// 本体のモデルを描いた後なので、そのフレームのボーンの位置がそのまま取れる
class SlashTrail : public Renderer {
private:
    struct Sample {
        VECTOR base;
        VECTOR tip;
        float age;
    };

    static constexpr int MAX_SAMPLES = 14;

    // 帯の長さ 長いほど残像が伸びる
    static constexpr float SAMPLE_LIFE = 0.14f;

    SkinnedMeshRenderer* _owner = nullptr;
    int _boneIndex = -1;
    VECTOR _baseLocal = VGet(0.0f, 0.0f, 0.0f);
    VECTOR _tipLocal = VGet(0.0f, 0.0f, 0.0f);
    int _graph = -1;
    bool _isEmitting = false;

    // 先頭が一番新しい
    std::vector<Sample> _samples;
    std::vector<VERTEX3D> _vertices;

public:
    COLOR_U8 trailColor = GetColorU8(255, 255, 255, 255);

    ~SlashTrail() override;

    // 刃の根元と先はボーンから見た位置で渡す
    bool Setup(SkinnedMeshRenderer* owner, const char* boneName,
        VECTOR baseLocal, VECTOR tipLocal, const char* texturePath);

    void SetEmitting(bool isEmitting) { _isEmitting = isEmitting; }

    void Render() override;

private:
    void Record();
    void Draw();
};
