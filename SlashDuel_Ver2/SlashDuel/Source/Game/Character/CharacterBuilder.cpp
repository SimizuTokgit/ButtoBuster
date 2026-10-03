#include "CharacterBuilder.h"
#include "ModelCache.h"
#include "GameObject.h"
#include "Animator.h"
#include "AnimationClip.h"
#include "SkinnedMeshRenderer.h"
#include "MeshRenderer.h"
#include "BoneFollower.h"
#include "SoundManager.h"
#include <memory>

namespace {
    // アニメは1秒30フレームで作られている
    constexpr float ANIMATION_FPS = 30.0f;
}

bool CharacterBuilder::LoadAnimations(Animator* animator, const std::string& folder, const ClipInfo* clips, int count) {
    if (!animator) return false;

    for (int i = 0; i < count; ++i) {
        const ClipInfo& info = clips[i];
        std::string path = folder + info.fileName;

        auto clip = std::make_unique<AnimationClip>();
        if (!clip->Load(path.c_str(), info.name, ANIMATION_FPS, info.isLoop)) return false;
        animator->RegisterAnimation(std::move(clip));
    }
    return true;
}

void CharacterBuilder::AddSound(Animator* animator, const char* clipName, float time, const char* soundName, float volume) {
    if (!animator) return;

    auto* clip = animator->GetClip(clipName);
    if (!clip) return;

    // 名前は文字列ごと持たせる 呼ばれるのはずっと後なので、元の文字列が残っているとは限らない
    std::string name = soundName;
    clip->AddEvent(time, [name, volume]() {
        SoundManager::Instance().PlaySE(name, volume);
    });
}

bool CharacterBuilder::AttachToBone(GameObject* model, SkinnedMeshRenderer* renderer, const char* boneName, const std::string& modelPath) {
    if (!model || !renderer) return false;

    // ボーンを追う入れ物を作り、その子に見た目を置く
    // 入れ物はボーンのワールドの位置をそのまま受け取るので、親の回転は二重にかからない
    auto* holder = model->AddChild(boneName);
    auto* follower = holder->AddComponent<BoneFollower>();
    if (!follower->Setup(renderer, boneName)) return false;

    auto* item = holder->AddChild("Item");
    auto* mesh = item->AddComponent<MeshRenderer>();
    return mesh->LoadDuplicate(ModelCache::Get(modelPath));
}
