#pragma once
#include <string>

class GameObject;
class Animator;
class SkinnedMeshRenderer;
class Rigidbody;
class CapsuleCollider;

// プレイヤーと敵の組み立てで共通する手順
namespace CharacterBuilder {

    // 体の当たり判定のカプセルの形
    struct BodyShape {
        float radius;
        float height;
        float centerY;      // 足元からカプセルの真ん中までの高さ
    };

    // 付けた体の部品 組み立ての最後に Character::Setup へ渡す
    struct Body {
        Rigidbody* rigidbody;
        CapsuleCollider* collider;
    };

    // 物理で動く体と、カプセルの当たり判定を付けて登録する
    Body AddBody(GameObject* root, bool useGravity, const BodyShape& shape);

    // 見た目のモデルを置く子を作る モデルの正面を体の正面にそろえて回しておく
    GameObject* AddModelRoot(GameObject* root, const char* name);

    struct ClipInfo {
        const char* fileName;   // Anim_Run.mv1 など
        const char* name;       // 登録名 状態からはこの名前で呼ぶ
        bool isLoop;
    };

    // フォルダの中のアニメをまとめて登録する
    bool LoadAnimations(Animator* animator, const std::string& folder, const ClipInfo* clips, int count);

    // アニメの決まった時間に音を鳴らす
    // 足音や振りの音をアニメに合わせたいので、状態のコードには書かない
    void AddSound(Animator* animator, const char* clipName, float time, const char* soundName, float volume = 1.0f);

    // ボーンに武器などのモデルを持たせる
    bool AttachToBone(GameObject* model, SkinnedMeshRenderer* renderer, const char* boneName, const std::string& modelPath);
}
