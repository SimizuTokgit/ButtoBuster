#include "EnemyFactory.h"
#include "Enemy.h"
#include "EnemyAI.h"
#include "CharacterBuilder.h"
#include "ModelCache.h"
#include "ColliderGizmo.h"
#include "Scene.h"
#include "GameObject.h"
#include "Rigidbody.h"
#include "CapsuleCollider.h"
#include "SkinnedMeshRenderer.h"
#include "Animator.h"
#include <string>

namespace {
    using CharacterBuilder::ClipInfo;

    // Goblin と RedGoblin はファイル名がそろっている
    const ClipInfo GOBLIN_CLIPS[] = {
        { "Anim_Neutral.mv1",   "Idle",     true  },
        { "Anim_Walk.mv1",      "Walk",     true  },
        { "Anim_Run.mv1",       "Run",      true  },
        { "Anim_Attack1.mv1",   "Attack1",  false },
        { "Anim_Damage.mv1",    "Damage",   false },
        { "Anim_Blow_In.mv1",   "BlowIn",   false },
        { "Anim_Down_Loop.mv1", "DownLoop", true  },
        { "Anim_Blow_Out.mv1",  "BlowOut",  false },
    };

    const ClipInfo BEE_CLIPS[] = {
        { "Anim_Neutral.mv1", "Idle",    true  },
        { "Anim_Walk.mv1",    "Walk",    true  },
        { "Anim_Run.mv1",     "Run",     true  },
        { "Anim_Attack1.mv1", "Attack1", false },
        { "Anim_Attack2.mv1", "Attack2", false },
        { "Anim_Damage.mv1",  "Damage",  false },
        { "Anim_Down.mv1",    "Down",    false },
    };

    // Golem は走りのアニメが無いので、歩きを走りの名前でも登録しておく
    // 状態のほうで種類ごとに分けずに済む
    const ClipInfo GOLEM_CLIPS[] = {
        { "Anim_Neutral.mv1", "Idle",    true  },
        { "Anim_Walk.mv1",    "Walk",    true  },
        { "Anim_Walk.mv1",    "Run",     true  },
        { "Anim_Attack1.mv1", "Attack1", false },
        { "Anim_Attack2.mv1", "Attack2", false },
        { "Anim_Attack3.mv1", "Attack3", false },
        { "Anim_Down.mv1",    "Down",    false },
    };

    template<int N>
    int CountOf(const ClipInfo (&)[N]) {
        return N;
    }

    bool LoadClips(EnemyKind kind, Animator* animator, const std::string& folder) {
        switch (kind) {
        case EnemyKind::Goblin:
        case EnemyKind::RedGoblin:
            return CharacterBuilder::LoadAnimations(animator, folder, GOBLIN_CLIPS, CountOf(GOBLIN_CLIPS));
        case EnemyKind::Bee:
            return CharacterBuilder::LoadAnimations(animator, folder, BEE_CLIPS, CountOf(BEE_CLIPS));
        case EnemyKind::Golem:
            return CharacterBuilder::LoadAnimations(animator, folder, GOLEM_CLIPS, CountOf(GOLEM_CLIPS));
        default:
            return false;
        }
    }

    // 鳴らす時間はアニメのフレーム 振りかぶりや踏み込みの動きに合わせてある
    // 振りかぶる声は、見えていなくても「来る」と分かる合図になる
    void RegisterSounds(EnemyKind kind, Animator* animator) {
        using CharacterBuilder::AddSound;

        switch (kind) {
        case EnemyKind::Goblin:
            AddSound(animator, "Attack1", 18.0f, "Goblin/VO_attack", 0.8f);
            AddSound(animator, "Attack1", 23.5f, "Weapon/Axe/swish");
            break;

        case EnemyKind::RedGoblin:
            AddSound(animator, "Attack1", 21.0f, "RedGoblin/VO_attack", 0.8f);
            AddSound(animator, "Attack1", 23.0f, "Weapon/Sword/swish");
            AddSound(animator, "Attack1", 33.0f, "Weapon/Sword/swish");
            break;

        case EnemyKind::Bee:
            AddSound(animator, "Attack1", 16.5f, "Bee/needle_shot");
            AddSound(animator, "Attack2", 0.5f, "Bee/VO_attack", 0.8f);
            AddSound(animator, "Attack2", 16.5f, "Bee/attack_sult_B");
            AddSound(animator, "Down", 0.0f, "Bee/flapDead");
            break;

        case EnemyKind::Golem:
            AddSound(animator, "Attack1", 23.5f, "Golem/attack_swish_S");
            AddSound(animator, "Attack1", 29.0f, "Golem/VO_attack");
            AddSound(animator, "Attack2", 23.0f, "Golem/attack_swish_S");
            AddSound(animator, "Attack2", 28.0f, "Golem/VO_attack");
            // 踏みつけの予備動作 この音を聞いたら離れるか回避する
            AddSound(animator, "Attack3", 13.0f, "Golem/attack_stompPre");
            AddSound(animator, "Attack3", 41.5f, "Golem/VO_attack_L");
            AddSound(animator, "Attack3", 50.5f, "Golem/attack_swish_L");
            AddSound(animator, "Attack3", 57.0f, "Golem/attack_stomp");
            AddSound(animator, "Down", 0.0f, "Golem/downing");
            break;

        default:
            break;
        }
    }
}

Enemy* EnemyFactory::Create(EnemyKind kind, VECTOR position, Character* target, int id) {
    const EnemyData& data = EnemyDatabase::Get(kind);
    Scene& scene = Scene::Instance();

    auto* root = scene.CreateGameObject(data.displayName);
    root->transform->localPosition = position;

    auto* rigidbody = root->AddComponent<Rigidbody>();
    rigidbody->useGravity = !data.isFlying;
    rigidbody->Register();

    auto* body = root->AddComponent<CapsuleCollider>();
    body->radius = data.bodyRadius * data.scale;
    body->height = data.bodyHeight * data.scale;
    body->center = VGet(0.0f, data.bodyCenterY * data.scale, 0.0f);
    body->Register();

    root->AddComponent<EnemyAI>();
    auto* enemy = root->AddComponent<Enemy>();
    root->AddComponent<ColliderGizmo>();
    enemy->Initialize(data, id, target);

    auto* model = root->AddChild("Model");
    // モデルは -Z を向いて作られているので半回転させて体の正面 +Z に合わせる
    model->transform->localEulerAngles = VGet(0.0f, 180.0f, 0.0f);
    model->transform->localScale = VGet(data.scale, data.scale, data.scale);

    std::string folder = data.folder;

    // 読み込みは種類ごとに1回だけ 2体目からは複製する
    auto* renderer = model->AddComponent<SkinnedMeshRenderer>();
    bool isBuilt = renderer->LoadDuplicate(ModelCache::Get(folder + data.modelFile));
    if (isBuilt) {
        renderer->SetupRootBone("root");
    }

    auto* animator = model->AddComponent<Animator>();
    isBuilt = isBuilt
        && animator->InitFromRenderer(renderer)
        && LoadClips(kind, animator, folder);

    if (!isBuilt) {
        // 途中まで組んだものは残さない
        scene.Destroy(root);
        return nullptr;
    }

    RegisterSounds(kind, animator);

    if (data.weaponBone && data.weaponModel) {
        CharacterBuilder::AttachToBone(model, renderer, data.weaponBone, data.weaponModel);
    }

    enemy->Setup(animator, renderer, rigidbody, body);
    return enemy;
}
