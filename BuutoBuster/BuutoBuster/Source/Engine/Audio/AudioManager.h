#pragma once
#include <vector>
#include <algorithm>

// 前方宣言
class AudioSource;
class AudioListener;

/// <summary>
/// AudioSource / AudioListener 管理シングルトンクラス
/// RenderManager / PhysicsManager と同じ設計パターン
/// </summary>
class AudioManager {
private:
    std::vector<AudioSource*> _sources;
    AudioListener* _activeListener = nullptr;

public:
    /// <summary>シングルトンインスタンスを取得</summary>
    static AudioManager& Instance() {
        static AudioManager instance;
        return instance;
    }

    // コピー禁止
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    /// <summary>AudioSourceを登録</summary>
    void RegisterSource(AudioSource* source) {
        if (source) {
            auto it = std::find(_sources.begin(), _sources.end(), source);
            if (it == _sources.end()) {
                _sources.push_back(source);
            }
        }
    }

    /// <summary>AudioSourceの登録解除</summary>
    void UnregisterSource(AudioSource* source) {
        auto it = std::find(_sources.begin(), _sources.end(), source);
        if (it != _sources.end()) {
            _sources.erase(it);
        }
    }

    /// <summary>アクティブなAudioListenerを設定</summary>
    void SetActiveListener(AudioListener* listener) {
        _activeListener = listener;
    }

    /// <summary>AudioListenerの登録解除</summary>
    void ClearActiveListener(AudioListener* listener) {
        if (_activeListener == listener) {
            _activeListener = nullptr;
        }
    }

    /// <summary>
    /// 毎フレーム更新
    /// リスナーとソースの3D位置を一括更新する
    /// </summary>
    void Update();

    /// <summary>アクティブなAudioListenerを取得</summary>
    AudioListener* GetActiveListener() const { return _activeListener; }

    /// <summary>登録されているAudioSource数を取得</summary>
    size_t GetSourceCount() const { return _sources.size(); }

    /// <summary>
    /// シーン切り替え時のクリア処理。
    /// AudioSource は各 GameObject のデストラクタ内で Unregister() が呼ばれるため、
    /// ここで _sources を一括クリアしてはいけない（SoundManager のように
    /// シーンに属さず長寿命なソースまで巻き込んでしまう）。
    /// リスナーのみクリアする。
    /// </summary>
    void Clear() {
        _activeListener = nullptr;
    }

private:
    AudioManager() = default;
    ~AudioManager() = default;
};
