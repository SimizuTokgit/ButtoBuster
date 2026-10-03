#pragma once
#include "Behaviour.h"
#include "Transform.h"
#include "AudioManager.h"
#include "DxLib.h"

/// <summary>
/// 音を聞くためのコンポーネント
/// UnityのAudioListenerに対応
/// シーン内に1つだけ存在する「耳」
/// CameraのGameObjectにアタッチする想定
/// </summary>
class AudioListener : public Behaviour {
private:
    bool _isRegistered = false;

public:
    AudioListener() = default;

    ~AudioListener() override {
        Unregister();
    }

    /// <summary>
    /// シーン内のアクティブなAudioListenerを取得
    /// </summary>
    static AudioListener* GetActive() {
        return AudioManager::Instance().GetActiveListener();
    }

    /// <summary>
    /// AudioManagerに登録
    /// </summary>
    void Register() {
        if (!_isRegistered) {
            AudioManager::Instance().SetActiveListener(this);
            _isRegistered = true;
        }
    }

    /// <summary>
    /// AudioManagerから登録解除
    /// </summary>
    void Unregister() {
        if (_isRegistered) {
            AudioManager::Instance().ClearActiveListener(this);
            _isRegistered = false;
        }
    }

    /// <summary>
    /// リスナーの3D位置を更新（AudioManagerから呼ばれる）
    /// transformの位置・前方ベクトルをDxLibの3Dリスナーに設定する
    /// </summary>
    void UpdateListenerPosition() {
        if (!enabled || !transform) return;

        VECTOR position = transform->position;

        // 前方ベクトルを計算
        VECTOR forward = transform->forward;
        VECTOR frontPosition = VAdd(position, forward);

        Set3DSoundListenerPosAndFrontPos_UpVecY(position, frontPosition);
    }
};
