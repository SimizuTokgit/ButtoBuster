#pragma once
#include "Behaviour.h"
#include "Transform.h"
#include "AudioClip.h"
#include "AudioManager.h"
#include "DxLib.h"
#include <vector>
#include <algorithm>

/// <summary>
/// 音を鳴らすためのコンポーネント
/// UnityのAudioSourceに対応
/// GameObjectにアタッチし、音声の再生・停止・パラメータ制御を行う
/// </summary>
class AudioSource : public Behaviour {
public:
    /// <summary>再生する音声クリップ</summary>
    AudioClip* clip = nullptr;

    /// <summary>true: Register時に自動再生</summary>
    bool playOnAwake = false;

    /// <summary>ループ再生するか</summary>
    bool loop = false;

    /// <summary>ミュートするか</summary>
    bool mute = false;

    /// <summary>音量 (0.0〜1.0)</summary>
    float volume = 1.0f;

    /// <summary>再生速度 (1.0=通常)</summary>
    float pitch = 1.0f;

    /// <summary>true: 3Dサウンド, false: 2Dサウンド</summary>
    bool spatialBlend = true;

    /// <summary>この距離以内は最大音量</summary>
    float minDistance = 100.0f;

    /// <summary>この距離以遠は無音</summary>
    float maxDistance = 10000.0f;

private:
    bool _isRegistered = false;

    /// <summary>PlayOneShotで作成した複製ハンドル（再生完了後に解放）</summary>
    std::vector<int> _oneShotHandles;

public:
    AudioSource() = default;

    ~AudioSource() override {
        Stop();
        CleanupOneShotHandles(true);
        Unregister();
    }

    /// <summary>
    /// 現在再生中かどうか
    /// </summary>
    bool IsPlaying() const {
        if (!clip || !clip->IsLoaded()) return false;
        return CheckSoundMem(clip->handle) == TRUE;
    }

    /// <summary>
    /// AudioManagerに登録
    /// </summary>
    void Register() {
        if (!_isRegistered) {
            AudioManager::Instance().RegisterSource(this);
            _isRegistered = true;

            if (playOnAwake && clip && clip->IsLoaded()) {
                Play();
            }
        }
    }

    /// <summary>
    /// AudioManagerから登録解除
    /// </summary>
    void Unregister() {
        if (_isRegistered) {
            AudioManager::Instance().UnregisterSource(this);
            _isRegistered = false;
        }
    }

    /// <summary>
    /// 再生開始
    /// </summary>
    void Play() {
        if (!clip || !clip->IsLoaded()) return;

        int handle = clip->handle;

        // 音量設定
        int vol = mute ? 0 : static_cast<int>(volume * 255.0f);
        if (vol < 0) vol = 0;
        if (vol > 255) vol = 255;
        ChangeVolumeSoundMem(vol, handle);

        // ピッチ（周波数）設定
        if (pitch != 1.0f) {
            int baseFreq = GetFrequencySoundMem(handle);
            if (baseFreq > 0) {
                SetFrequencySoundMem(static_cast<int>(baseFreq * pitch), handle);
            }
        }

        // 3Dサウンドパラメータ設定
        if (spatialBlend && transform) {
            Set3DPositionSoundMem(transform->position, handle);
            Set3DRadiusSoundMem(maxDistance, handle);
        }

        // 再生
        int playType = loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK;
        PlaySoundMem(handle, playType, TRUE);
    }

    /// <summary>
    /// 指定したAudioClipを一度だけ再生（効果音向け）
    /// UnityのPlayOneShotに対応
    /// 現在のclipの再生を妨げない
    /// </summary>
    /// <param name="oneShotClip">再生するクリップ</param>
    /// <param name="volumeScale">音量スケール (0.0〜1.0)</param>
    void PlayOneShot(AudioClip* oneShotClip, float volumeScale = 1.0f) {
        if (!oneShotClip || !oneShotClip->IsLoaded()) return;

        // サウンドハンドルを複製して再生
        int dupHandle = DuplicateSoundMem(oneShotClip->handle);
        if (dupHandle == -1) return;

        // 音量設定
        float finalVolume = volume * volumeScale;
        if (mute) finalVolume = 0.0f;
        int vol = static_cast<int>(finalVolume * 255.0f);
        if (vol < 0) vol = 0;
        if (vol > 255) vol = 255;
        ChangeVolumeSoundMem(vol, dupHandle);

        // 3Dサウンドパラメータ設定
        if (spatialBlend && transform) {
            Set3DPositionSoundMem(transform->position, dupHandle);
            Set3DRadiusSoundMem(maxDistance, dupHandle);
        }

        PlaySoundMem(dupHandle, DX_PLAYTYPE_BACK, TRUE);
        _oneShotHandles.push_back(dupHandle);
    }

    /// <summary>
    /// 再生停止
    /// </summary>
    void Stop() {
        if (clip && clip->IsLoaded()) {
            StopSoundMem(clip->handle);
        }
    }

    /// <summary>
    /// 一時停止
    /// </summary>
    void Pause() {
        // DxLibにはPause APIがないため、停止で代用
        // ※ 再開時は先頭から再生される制限あり
        Stop();
    }

    /// <summary>
    /// 一時停止から再開
    /// </summary>
    void UnPause() {
        Play();
    }

    /// <summary>
    /// ソースの3D位置を更新（AudioManagerから呼ばれる）
    /// </summary>
    void UpdateSourcePosition() {
        if (!enabled) return;

        // 再生中のclipの3D位置を更新
        if (clip && clip->IsLoaded() && spatialBlend && transform) {
            if (CheckSoundMem(clip->handle) == TRUE) {
                Set3DPositionSoundMem(transform->position, clip->handle);
                Set3DRadiusSoundMem(maxDistance, clip->handle);

                int vol = mute ? 0 : static_cast<int>(volume * 255.0f);
                if (vol < 0) vol = 0;
                if (vol > 255) vol = 255;
                ChangeVolumeSoundMem(vol, clip->handle);
            }
        }

        // PlayOneShotの複製ハンドルも3D位置を更新
        if (spatialBlend && transform) {
            for (int h : _oneShotHandles) {
                if (CheckSoundMem(h) == TRUE) {
                    Set3DPositionSoundMem(transform->position, h);
                }
            }
        }

        // 再生完了したOneShotハンドルを解放
        CleanupOneShotHandles(false);
    }

private:
    /// <summary>
    /// 再生完了したOneShotハンドルを解放する
    /// </summary>
    /// <param name="forceAll">trueなら全て強制解放</param>
    void CleanupOneShotHandles(bool forceAll) {
        auto it = _oneShotHandles.begin();
        while (it != _oneShotHandles.end()) {
            if (forceAll || CheckSoundMem(*it) != TRUE) {
                DeleteSoundMem(*it);
                it = _oneShotHandles.erase(it);
            }
            else {
                ++it;
            }
        }
    }
};
