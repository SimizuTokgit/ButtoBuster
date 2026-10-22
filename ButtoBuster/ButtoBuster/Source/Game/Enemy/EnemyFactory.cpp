#include "EnemyFactory.h"
#include "Enemy.h"
#include "EnemyAI.h"
#include "CharacterBuilder.h"
#include "ModelCache.h"
#include "ColliderGizmo.h"
#include "Scene.h"
#include "GameObject.h"
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

    // 振りかぶる声の大きさ
    constexpr float ATTACK_VOICE_VOLUME = 0.8f;

    // ----- 音を鳴らすアニメのフレーム (RegisterSounds) 振りかぶりや踏み込みの動きに合わせてある -----
    // 振りかぶる声と、振る音
    constexpr struct {
        float voice;
        float swing;
    } GOBLIN_SOUND_FRAMES = { 18.0f, 23.5f };

    // 振りかぶる声と、二段斬りの 1 振り目と 2 振り目の音
    constexpr struct {
        float voice;
        float swing;
        float swing2;
    } RED_GOBLIN_SOUND_FRAMES = { 21.0f, 23.0f, 33.0f };

    // 針を撃つ音 (Attack1) と、急降下して刺すときの声と音 (Attack2)
    constexpr struct {
        float shoot;
        float stabVoice;
        float stab;
    } BEE_SOUND_FRAMES = { 16.5f, 0.5f, 16.5f };

    // 2 つの振り (Attack1 Attack2) の音と声 踏みつけ (Attack3) の予備動作の音 声 振る音 踏む音
    constexpr struct {
        float swing;
        float voice;
        float swingAlt;
        float voiceAlt;
        float stompSign;
        float stompVoice;
        float stompSwish;
        float stomp;
    } GOLEM_SOUND_FRAMES = { 23.5f, 29.0f, 23.0f, 28.0f, 13.0f, 41.5f, 50.5f, 57.0f };

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

    // アニメの決まったフレームに音を鳴らす
    // 振りかぶる声は、見えていなくても「来る」と分かる合図になる
    void RegisterSounds(EnemyKind kind, Animator* animator) {
        using CharacterBuilder::AddSound;

        switch (kind) {
        case EnemyKind::Goblin:
            AddSound(animator, "Attack1", GOBLIN_SOUND_FRAMES.voice, "Goblin/Vc_Goblin_Attack", ATTACK_VOICE_VOLUME);
            AddSound(animator, "Attack1", GOBLIN_SOUND_FRAMES.swing, "Goblin/SE_Goblin_Attack");
            break;

        case EnemyKind::RedGoblin:
            // 二段斬り 1 振り目と 2 振り目で音を変える
            AddSound(animator, "Attack1", RED_GOBLIN_SOUND_FRAMES.voice, "RedGoblin/Vc_RedGoblin_Attack", ATTACK_VOICE_VOLUME);
            AddSound(animator, "Attack1", RED_GOBLIN_SOUND_FRAMES.swing, "RedGoblin/SE_RedGoblin_Attack1");
            AddSound(animator, "Attack1", RED_GOBLIN_SOUND_FRAMES.swing2, "RedGoblin/SE_RedGoblin_Attack2");
            break;

        case EnemyKind::Bee:
            AddSound(animator, "Attack1", BEE_SOUND_FRAMES.shoot, "Bee/SE_Bee_Attack");
            AddSound(animator, "Attack2", BEE_SOUND_FRAMES.stabVoice, "Bee/Vc_Bee_Attack", ATTACK_VOICE_VOLUME);
            AddSound(animator, "Attack2", BEE_SOUND_FRAMES.stab, "Bee/attack_sult_B");
            AddSound(animator, "Down", 0.0f, "Bee/flapDead");
            break;

        case EnemyKind::Golem:
            AddSound(animator, "Attack1", GOLEM_SOUND_FRAMES.swing, "Golem/SE_Golem_Attack");
            AddSound(animator, "Attack1", GOLEM_SOUND_FRAMES.voice, "Golem/VO_attack");
            AddSound(animator, "Attack2", GOLEM_SOUND_FRAMES.swingAlt, "Golem/SE_Golem_Attack");
            AddSound(animator, "Attack2", GOLEM_SOUND_FRAMES.voiceAlt, "Golem/VO_attack");
            // 踏みつけの予備動作 この音を聞いたら離れるか回避する
            AddSound(animator, "Attack3", GOLEM_SOUND_FRAMES.stompSign, "Golem/attack_stompPre");
            AddSound(animator, "Attack3", GOLEM_SOUND_FRAMES.stompVoice, "Golem/VO_attack_L");
            AddSound(animator, "Attack3", GOLEM_SOUND_FRAMES.stompSwish, "Golem/attack_swish_L");
            AddSound(animator, "Attack3", GOLEM_SOUND_FRAMES.stomp, "Golem/SE_Golem_SpAttack");
            AddSound(animator, "Down", 0.0f, "Golem/downing");
            break;

        default:
            break;
        }
    }
}

Enemy* EnemyFactory::Create(EnemyKind kind, VECTOR position, Character* target, int id) {
    return Create(EnemyDatabase::Get(kind), position, target, id);
}

Enemy* EnemyFactory::Create(const EnemyData& data, VECTOR position, Character* target, int id) {
    EnemyKind kind = data.kind;
    Scene& scene = Scene::Instance();

    auto* root = scene.CreateGameObject(data.displayName);
    root->transform->localPosition = position;

    // 体の大きさは見た目と同じ倍率で変える 空を飛ぶ敵は重力を受けない
    CharacterBuilder::BodyShape shape = {
        data.bodyRadius * data.scale,
        data.bodyHeight * data.scale,
        data.bodyCenterY * data.scale,
    };
    auto body = CharacterBuilder::AddBody(root, !data.isFlying, shape);

    root->AddComponent<EnemyAI>();
    auto* enemy = root->AddComponent<Enemy>();
    root->AddComponent<ColliderGizmo>();
    enemy->Initialize(data, id, target);

    auto* model = CharacterBuilder::AddModelRoot(root, "Model");
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

    enemy->Setup(animator, renderer, body.rigidbody, body.collider);
    return enemy;
}
