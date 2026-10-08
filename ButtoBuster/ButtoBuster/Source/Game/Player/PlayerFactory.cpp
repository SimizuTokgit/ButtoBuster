#include "PlayerFactory.h"
#include "Player.h"
#include "PlayerController.h"
#include "ChargeSoundObserver.h"
#include "ChargeEffectObserver.h"
#include "ChainSoundObserver.h"
#include "ChainEffectObserver.h"
#include "JustDodgeSoundObserver.h"
#include "JustDodgeEffectObserver.h"
#include "CharacterBuilder.h"
#include "SlashTrail.h"
#include "ColliderGizmo.h"
#include "Scene.h"
#include "GameObject.h"
#include "Rigidbody.h"
#include "CapsuleCollider.h"
#include "SkinnedMeshRenderer.h"
#include "Animator.h"

namespace {
    const char* const FOLDER = "Data/Character/Player/";

    const CharacterBuilder::ClipInfo CLIPS[] = {
        { "Anim_Neutral.mv1",      "Neutral",     true  },
        { "Anim_Run.mv1",          "Run",         true  },
        { "Anim_Attack1.mv1",      "Attack1",     false },
        { "Anim_Attack2.mv1",      "Attack2",     false },
        { "Anim_Attack3.mv1",      "Attack3",     false },
        { "Anim_Jump_In.mv1",      "JumpIn",      false },
        { "Anim_Jump_Loop.mv1",    "JumpLoop",    true  },
        { "Anim_Jump_Out.mv1",     "JumpOut",     false },
        { "Anim_Guard_In.mv1",     "GuardIn",     false },
        { "Anim_Guard_Loop.mv1",   "GuardLoop",   true  },
        { "Anim_Guard_Out.mv1",    "GuardOut",    false },
        { "Anim_Guard_Impact.mv1", "GuardImpact", false },
        { "Anim_Damage.mv1",       "Damage",      false },
        { "Anim_Blow_In.mv1",      "BlowIn",      false },
        { "Anim_Down_Loop.mv1",    "DownLoop",    true  },
        { "Anim_Blow_Out.mv1",     "BlowOut",     false },
    };

    // 鳴らす時間はアニメのフレーム 足が着く瞬間や剣を振る瞬間に合わせてある
    void RegisterSounds(Animator* animator) {
        using CharacterBuilder::AddSound;

        AddSound(animator, "Run", 0.5f, "Player/MAT_footstep_grass", 0.5f);
        AddSound(animator, "Run", 8.5f, "Player/MAT_footstep_grass", 0.5f);

        AddSound(animator, "Attack1", 3.5f, "Weapon/Sabel/swish_S");
        AddSound(animator, "Attack1", 5.5f, "Player/VO_J_attack", 0.7f);

        AddSound(animator, "Attack2", 3.0f, "Player/VO_J_attack", 0.7f);
        AddSound(animator, "Attack2", 4.0f, "Weapon/Sabel/swish_S");

        AddSound(animator, "Attack3", 6.5f, "Weapon/Sabel/swish_L2");
        AddSound(animator, "Attack3", 7.0f, "Player/VO_J_attack_L", 0.8f);
        AddSound(animator, "Attack3", 8.0f, "Weapon/Sabel/swish_L");

        AddSound(animator, "GuardIn", 0.0f, "Player/guard_In");
        AddSound(animator, "GuardIn", 1.5f, "Player/guard_On", 0.6f);
        AddSound(animator, "GuardOut", 0.0f, "Player/guard_Off");

        AddSound(animator, "JumpIn", 0.0f, "Player/jumpIn_B");
        AddSound(animator, "JumpIn", 1.0f, "Player/VO_J_jump", 0.7f);
        AddSound(animator, "JumpOut", 0.0f, "Player/MAT_footstep_grass");

        AddSound(animator, "BlowOut", 5.0f, "Player/equip");
    }
}

Player* PlayerFactory::Create(VECTOR position) {
    Scene& scene = Scene::Instance();

    auto* root = scene.CreateGameObject("Player");
    root->transform->localPosition = position;

    auto* rigidbody = root->AddComponent<Rigidbody>();
    rigidbody->useGravity = true;
    rigidbody->Register();

    auto* body = root->AddComponent<CapsuleCollider>();
    body->radius = 30.0f;
    body->height = 160.0f;
    body->center = VGet(0.0f, 80.0f, 0.0f);
    body->Register();

    // 操作役を先に付けておくと、毎フレーム Player より先に入力を作る
    root->AddComponent<PlayerController>();
    auto* player = root->AddComponent<Player>();
    root->AddComponent<ColliderGizmo>();

    // 溜めの音と光は Player に書かず、知らせを受け取る側としてここでつなぐ
    // 演出を足すときも Player と溜めの状態は書き換えずに済む
    player->GetChargeEvents().AddObserver(root->AddComponent<ChargeSoundObserver>());
    player->GetChargeEvents().AddObserver(root->AddComponent<ChargeEffectObserver>());

    // 連鎖ぶっ飛ばしの音と揺れも同じ形でつなぐ
    player->GetChainEvents().AddObserver(root->AddComponent<ChainSoundObserver>());
    player->GetChainEvents().AddObserver(root->AddComponent<ChainEffectObserver>());

    // ジャスト回避の音と画面の演出も同じ形でつなぐ
    player->GetJustDodgeEvents().AddObserver(root->AddComponent<JustDodgeSoundObserver>());
    player->GetJustDodgeEvents().AddObserver(root->AddComponent<JustDodgeEffectObserver>());

    auto* model = root->AddChild("PlayerModel");

    // モデルは -Z を向いて作られているので、体の正面 +Z に合わせて半回転させる
    model->transform->localEulerAngles = VGet(0.0f, 180.0f, 0.0f);

    auto* renderer = model->AddComponent<SkinnedMeshRenderer>();
    if (!renderer->Load("Data/Character/Player/PC.mv1")) return nullptr;

    auto* animator = model->AddComponent<Animator>();
    if (!animator->InitFromRenderer(renderer)) return nullptr;

    int clipCount = static_cast<int>(sizeof(CLIPS) / sizeof(CLIPS[0]));
    if (!CharacterBuilder::LoadAnimations(animator, FOLDER, CLIPS, clipCount)) return nullptr;
    RegisterSounds(animator);

    CharacterBuilder::AttachToBone(model, renderer, "wp", "Data/Character/Weapon/Sabel/Sabel.mv1");
    CharacterBuilder::AttachToBone(model, renderer, "sayabone", "Data/Character/Player/Saya.mv1");

    // 刃は wp ボーンから下に 100 伸びている
    auto* trailObject = model->AddChild("SlashTrail");
    auto* trail = trailObject->AddComponent<SlashTrail>();
    trail->trailColor = GetColorU8(170, 220, 255, 255);
    trail->Setup(renderer, "wp", VGet(0.0f, -20.0f, 0.0f), VGet(0.0f, -110.0f, 0.0f), "Data/Effect/SlashLocus.png");
    player->SetTrail(trail);

    player->Setup(animator, renderer, rigidbody, body);
    return player;
}
