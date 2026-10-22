#include "CharacterBuilder.h"
#include "ModelCache.h"
#include "GameObject.h"
#include "Animator.h"
#include "AnimationClip.h"
#include "SkinnedMeshRenderer.h"
#include "MeshRenderer.h"
#include "BoneFollower.h"
#include "SoundManager.h"
#include "AttackData.h"
#include "Rigidbody.h"
#include "CapsuleCollider.h"
#include <memory>

namespace {
    // モデルは -Z を向いて作られているので、Y 軸でこの角度 (度) 回して体の正面 +Z に合わせる
    constexpr float MODEL_FACING_YAW = 180.0f;
}

CharacterBuilder::Body CharacterBuilder::AddBody(GameObject* root, bool useGravity, const BodyShape& shape) {
    auto* rigidbody = root->AddComponent<Rigidbody>();
    rigidbody->useGravity = useGravity;
    rigidbody->Register();

    auto* collider = root->AddComponent<CapsuleCollider>();
    collider->radius = shape.radius;
    collider->height = shape.height;
    collider->center = VGet(0.0f, shape.centerY, 0.0f);
    collider->Register();
    return { rigidbody, collider };
}

GameObject* CharacterBuilder::AddModelRoot(GameObject* root, const char* name) {
    auto* model = root->AddChild(name);
    model->transform->localEulerAngles = VGet(0.0f, MODEL_FACING_YAW, 0.0f);
    return model;
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
