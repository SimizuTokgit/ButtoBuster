#pragma once
#include "Behaviour.h"

#include "AnimationClip.h"
#include "DxLib.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <memory>

// 前方宣言
class SkinnedMeshRenderer;

/// <summary>
/// アニメーション管理コンポーネント
/// UnityのAnimator相当
/// SkinnedMeshRendererと連携してアニメーションを再生
/// </summary>
class Animator : public Behaviour {
public:
    /// <summary>モデルハンドル（準定数）</summary>
    int ModelHandle = -1;

    /// <summary>ルートフレーム（準定数）</summary>
    int RootFrame = -1;

    /// <summary>関連するSkinnedMeshRenderer</summary>
    SkinnedMeshRenderer* skinnedMeshRenderer = nullptr;

private:
    std::unordered_map<std::string, std::unique_ptr<AnimationClip>> _animations;
    std::string _currentAnimName;
    int _attachIndex = -1;
    float _animPlayTime = 0.0f;

public:
    Animator() = default;
    ~Animator() override = default;

    /// <summary>
    /// 既にロード済みのモデルハンドルを受け取って初期化
    /// </summary>
    /// <param name="handle">モデルハンドル</param>
    /// <param name="rootName">ルートボーン名</param>
    /// <returns>成功ならtrue</returns>
    bool Init(int handle, const char* rootName) {
        ModelHandle = handle;
        if (ModelHandle == -1) return false;

        RootFrame = MV1SearchFrame(ModelHandle, rootName);
        if (RootFrame == -1) return false;

        MV1SetFrameUserLocalMatrix(ModelHandle, RootFrame, MGetIdent());

        _animations.clear();
        _currentAnimName.clear();
        _attachIndex = -1;
        _animPlayTime = 0.0f;

        return true;
    }

    /// <summary>
    /// SkinnedMeshRendererから初期化
    /// </summary>
    /// <param name="renderer">SkinnedMeshRenderer</param>
    /// <returns>成功ならtrue</returns>
    bool InitFromRenderer(SkinnedMeshRenderer* renderer);

    /// <summary>
    /// アニメーションクリップを登録
    /// </summary>
    bool RegisterAnimation(std::unique_ptr<AnimationClip> animationClip) {
        _animations[animationClip->Name] = std::move(animationClip);
        return true;
    }

    /// <summary>
    /// アニメーションを再生
    /// </summary>
    /// <param name="name">アニメーション名</param>
    /// <param name="deltaTime">経過時間</param>
    void Play(const std::string& name, float deltaTime) {
        if (!enabled) return;

        auto it = _animations.find(name);
        if (it == _animations.end()) return;

        auto* clip = it->second.get();

        if (_currentAnimName != name || _attachIndex == -1) {
            if (_attachIndex != -1) {
                MV1DetachAnim(ModelHandle, _attachIndex);
            }

            _attachIndex = MV1AttachAnim(ModelHandle, 0, clip->Handle);
            if (_attachIndex == -1) return;

            clip->TotalTime = MV1GetAttachAnimTotalTime(ModelHandle, _attachIndex);
            _animPlayTime = 0.0f;
            _currentAnimName = name;
        }

        if (clip->TotalTime <= 0.0f) return;

        float prevTime = _animPlayTime;

        if (clip->IsLoop) {
            _animPlayTime = fmod(_animPlayTime + clip->Speed * deltaTime, clip->TotalTime);
        }
        else {
            _animPlayTime += clip->Speed * deltaTime;
            if (_animPlayTime > clip->TotalTime) _animPlayTime = clip->TotalTime;
        }

        // AnimationEvent detection
        if (!clip->events.empty()) {
            static constexpr float TIME_SCALE = 10000.0f;
            int prevI = static_cast<int>(prevTime * TIME_SCALE);
            int currI = static_cast<int>(_animPlayTime * TIME_SCALE);

            for (const auto& evt : clip->events) {
                int evtI = static_cast<int>(evt.time * TIME_SCALE);

                if (clip->IsLoop && prevI > currI) {
                    // Loop wraparound: fire if NOT in [currI, prevI)
                    if (evtI >= currI && evtI < prevI) continue;
                }
                else {
                    // Normal: fire if in [prevI, currI)
                    if (evtI < prevI || evtI >= currI) continue;
                }

                if (evt.callback) evt.callback();
            }
        }

        MV1SetAttachAnimTime(ModelHandle, _attachIndex, _animPlayTime);
    }

    /// <summary>
    /// 現在のアニメーションを先頭に巻き戻す
    ///
    /// Play() は同じアニメーション名を指定しても再生位置を維持するため、
    /// 「同じモーションをもう一度頭から再生したい」場合はこれを呼ぶ。
    /// 例: 敵が連続で同じ攻撃を出すとき
    /// </summary>
    void Rewind() {
        _animPlayTime = 0.0f;

        if (ModelHandle != -1 && _attachIndex != -1) {
            MV1SetAttachAnimTime(ModelHandle, _attachIndex, 0.0f);
        }
    }

    /// <summary>
    /// 現在のアニメーション名を取得
    /// </summary>
    const std::string& GetCurrentAnimationName() const { return _currentAnimName; }

    /// <summary>
    /// 現在のアニメーション再生時間を取得
    /// </summary>
    float GetCurrentTime() const { return _animPlayTime; }

    /// <summary>
    /// アニメーションが終了したか（非ループ時）
    /// </summary>
    bool IsFinished() const {
        auto it = _animations.find(_currentAnimName);
        if (it == _animations.end()) return true;

        auto* clip = it->second.get();
        if (clip->IsLoop) return false;

        return _animPlayTime >= clip->TotalTime;
    }

    /// <summary>
    /// 登録済みのAnimationClipを名前で取得する
    /// </summary>
    AnimationClip* GetClip(const std::string& name) {
        auto it = _animations.find(name);
        if (it == _animations.end()) return nullptr;
        return it->second.get();
    }
};

// SkinnedMeshRendererとの連携用インライン関数
#include "SkinnedMeshRenderer.h"

inline bool Animator::InitFromRenderer(SkinnedMeshRenderer* renderer) {
    if (!renderer || !renderer->IsLoaded()) return false;

    skinnedMeshRenderer = renderer;
    ModelHandle = renderer->ModelHandle;
    RootFrame = renderer->RootFrame;

    if (RootFrame == -1) {
        RootFrame = MV1SearchFrame(ModelHandle, "root");
    }

    if (RootFrame != -1) {
        MV1SetFrameUserLocalMatrix(ModelHandle, RootFrame, MGetIdent());
    }

    _animations.clear();
    _currentAnimName.clear();
    _attachIndex = -1;
    _animPlayTime = 0.0f;

    renderer->animator = this;

    return true;
}
