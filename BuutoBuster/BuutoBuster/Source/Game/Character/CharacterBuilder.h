#pragma once
#include "DxLib.h"
#include <string>

class GameObject;
class Animator;
class SkinnedMeshRenderer;

// プレイヤーと敵の組み立てで共通する手順
namespace CharacterBuilder {

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
