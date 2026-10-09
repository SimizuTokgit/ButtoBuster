#include "SoundManager.h"
#include "DxLib.h"
#include "GameObject.h"
#include "AudioSource.h"
#include <cctype>
#include <algorithm>

SoundManager::SoundManager() = default;
SoundManager::~SoundManager() = default;

void SoundManager::Init() {
    if (_initialized) return;

    // サウンド専用 GameObject を生成（Scene には属さない）
    _soundGameObject = std::make_unique<GameObject>("SoundManager");

    // SE 用 AudioSource プール
    for (int i = 0; i < SE_SOURCE_COUNT; ++i) {
        auto* src = _soundGameObject->AddComponent<AudioSource>();
        src->spatialBlend = false;
        src->Register();
        _seSources.push_back(src);
    }

    // BGM 用 AudioSource プール
    for (int i = 0; i < BGM_SOURCE_COUNT; ++i) {
        auto* src = _soundGameObject->AddComponent<AudioSource>();
        src->spatialBlend = false;
        src->Register();
        _bgmSources.push_back(src);
    }

    // クリップ一括ロード
    LoadFolder("Data/Sound/SE", "", _seClips, true);
    LoadFolder("Data/Sound/BGM", "", _bgmClips, false);

    // 新しく作った音 SE と声は SE と同じように、フォルダ込みの名前で引く 例 Goblin/Vc_Goblin_Spw  Player/SE_Attack1
    LoadFolder("Data/Sound/ButtoBuster_SE&BGM/SE", "", _seClips, true);
    LoadFolder("Data/Sound/ButtoBuster_SE&BGM/Voice", "", _seClips, true);
    LoadFolder("Data/Sound/ButtoBuster_SE&BGM/BGM", "", _bgmClips, false);

    _initialized = true;
}

void SoundManager::Terminate() {
    // 進行中フェードを破棄（ソース破棄前に）
    _bgmFades.clear();

    // ポインタ参照をクリア（所有は _soundGameObject 側）
    _seSources.clear();
    _bgmSources.clear();

    // GameObject を破棄 → AudioSource デストラクタで Stop() と AudioManager から Unregister() される
    _soundGameObject.reset();

    // unique_ptr のデストラクタで AudioClip::Unload() される
    _seClips.clear();
    _bgmClips.clear();
    _seGroups.clear();
    _lastPlayedTimes.clear();
    _initialized = false;
}

void SoundManager::PlayJingle(const std::string& jingleName, float volumeScale) {
    auto it = _bgmClips.find(jingleName);
    if (it == _bgmClips.end() || !it->second || !it->second->IsLoaded()) return;

    AudioSource* src = GetUnusedBGMSource();
    if (!src) return;

    src->clip = it->second.get();
    src->loop = false;              // ここが PlayBGM との違い
    src->spatialBlend = false;
    src->mute = false;
    src->volume = ClampVolume(_bgmVolume * volumeScale);
    src->Play();
}

void SoundManager::PlaySE(const std::string& seName, float volumeScale) {
    const std::string* resolved = ResolveSEName(seName);
    if (!resolved) return;

    auto it = _seClips.find(*resolved);
    if (it == _seClips.end() || !it->second || !it->second->IsLoaded()) return;

    // 同名 SE の連続再生抑制
    // 呼ばれた名前で見るので、番号違いを順に鳴らして抑制をすり抜けることはない
    float now = static_cast<float>(GetNowCount()) / 1000.0f;
    auto lastIt = _lastPlayedTimes.find(seName);
    if (lastIt != _lastPlayedTimes.end() && now - lastIt->second < INTERVAL) return;

    AudioSource* src = GetUnusedSESource();
    if (!src) return;

    src->clip = it->second.get();
    src->loop = false;
    src->spatialBlend = false;
    src->mute = false;
    src->volume = ClampVolume(_seVolume * volumeScale);
    src->Play();

    _lastPlayedTimes[seName] = now;
}

void SoundManager::PlayBGM(const std::string& bgmName, float fadeInDuration) {
    auto it = _bgmClips.find(bgmName);
    if (it == _bgmClips.end() || !it->second || !it->second->IsLoaded()) return;

    float now = static_cast<float>(GetNowCount()) / 1000.0f;
    auto lastIt = _lastPlayedTimes.find(bgmName);
    if (lastIt != _lastPlayedTimes.end() && now - lastIt->second < INTERVAL) return;

    AudioSource* src = GetUnusedBGMSource();
    if (!src) return;

    src->clip = it->second.get();
    src->loop = true;
    src->spatialBlend = false;
    src->mute = false;
    src->volume = (fadeInDuration > 0.0f) ? 0.0f : _bgmVolume;
    src->Play();

    if (fadeInDuration > 0.0f) {
        StartBGMFade(src, 0.0f, _bgmVolume, fadeInDuration, false);
    }

    _lastPlayedTimes[bgmName] = now;
}

void SoundManager::StopBGM(const std::string& bgmName, float fadeOutDuration) {
    auto it = _bgmClips.find(bgmName);
    if (it == _bgmClips.end() || !it->second || !it->second->IsLoaded()) return;

    AudioClip* targetClip = it->second.get();
    for (auto* src : _bgmSources) {
        if (src && src->clip == targetClip) {
            if (fadeOutDuration > 0.0f) {
                StartBGMFade(src, src->volume, 0.0f, fadeOutDuration, true);
            }
            else {
                RemoveBGMFade(src);
                src->Stop();
            }
        }
    }
}

void SoundManager::StopAllBGM(float fadeOutDuration) {
    for (auto* src : _bgmSources) {
        if (!src) continue;
        if (!src->IsPlaying()) continue;

        if (fadeOutDuration > 0.0f) {
            StartBGMFade(src, src->volume, 0.0f, fadeOutDuration, true);
        }
        else {
            RemoveBGMFade(src);
            src->Stop();
        }
    }
}

void SoundManager::CrossfadeBGM(const std::string& bgmName, float duration) {
    auto it = _bgmClips.find(bgmName);
    if (it == _bgmClips.end() || !it->second || !it->second->IsLoaded()) return;

    AudioClip* targetClip = it->second.get();

    // 既に同じ BGM が鳴っているなら何もしない
    for (auto* src : _bgmSources) {
        if (src && src->clip == targetClip && src->IsPlaying()) {
            return;
        }
    }

    // 全ての再生中 BGM をフェードアウト指示
    for (auto* src : _bgmSources) {
        if (src && src->IsPlaying()) {
            StartBGMFade(src, src->volume, 0.0f, duration, true);
        }
    }

    // フェードイン用の空きソースを取得（無ければ強制的に 1 本回収）
    AudioSource* newSrc = GetUnusedBGMSource();
    if (!newSrc) {
        // 全ソース使用中: 先頭ソースを強制停止して再利用
        for (auto* src : _bgmSources) {
            if (src) {
                RemoveBGMFade(src);
                src->Stop();
                newSrc = src;
                break;
            }
        }
    }
    if (!newSrc) return;

    newSrc->clip = targetClip;
    newSrc->loop = true;
    newSrc->spatialBlend = false;
    newSrc->mute = false;
    newSrc->volume = 0.0f;
    newSrc->Play();
    StartBGMFade(newSrc, 0.0f, _bgmVolume, duration, false);

    float now = static_cast<float>(GetNowCount()) / 1000.0f;
    _lastPlayedTimes[bgmName] = now;
}

void SoundManager::Update(float deltaTime) {
    auto it = _bgmFades.begin();
    while (it != _bgmFades.end()) {
        it->elapsed += deltaTime;
        float t = (it->duration > 0.0f) ? (it->elapsed / it->duration) : 1.0f;
        if (t > 1.0f) t = 1.0f;
        float vol = it->startVolume + (it->targetVolume - it->startVolume) * t;

        if (it->source) {
            it->source->volume = vol;
            // 再生中ハンドルにも即時反映
            if (it->source->clip && it->source->clip->IsLoaded()) {
                int v = static_cast<int>(vol * 255.0f);
                if (v < 0) v = 0;
                if (v > 255) v = 255;
                ChangeVolumeSoundMem(v, it->source->clip->handle);
            }
        }

        if (t >= 1.0f) {
            if (it->stopOnComplete && it->source) {
                it->source->Stop();
            }
            it = _bgmFades.erase(it);
        }
        else {
            ++it;
        }
    }
}

void SoundManager::StartBGMFade(AudioSource* src, float startVol, float targetVol,
    float duration, bool stopOnComplete) {
    if (!src) return;
    RemoveBGMFade(src);

    BGMFade fade;
    fade.source = src;
    fade.startVolume = startVol;
    fade.targetVolume = targetVol;
    fade.elapsed = 0.0f;
    fade.duration = duration;
    fade.stopOnComplete = stopOnComplete;
    _bgmFades.push_back(fade);

    src->volume = startVol;
}

void SoundManager::RemoveBGMFade(AudioSource* src) {
    _bgmFades.erase(
        std::remove_if(_bgmFades.begin(), _bgmFades.end(),
            [src](const BGMFade& f) { return f.source == src; }),
        _bgmFades.end());
}

AudioClip* SoundManager::GetSEClip(const std::string& seName) {
    const std::string* resolved = ResolveSEName(seName);
    if (!resolved) return nullptr;

    auto it = _seClips.find(*resolved);
    return (it != _seClips.end()) ? it->second.get() : nullptr;
}

const std::string* SoundManager::ResolveSEName(const std::string& seName) const {
    auto exact = _seClips.find(seName);
    if (exact != _seClips.end()) return &exact->first;

    auto group = _seGroups.find(seName);
    if (group == _seGroups.end() || group->second.empty()) return nullptr;

    const auto& names = group->second;
    int index = GetRand(static_cast<int>(names.size()) - 1);
    return &names[index];
}

AudioClip* SoundManager::GetBGMClip(const std::string& bgmName) {
    auto it = _bgmClips.find(bgmName);
    return (it != _bgmClips.end()) ? it->second.get() : nullptr;
}

void SoundManager::SetSEVolume(float v) {
    _seVolume = ClampVolume(v);
    // 再生中の SE にも即時反映
    for (auto* src : _seSources) {
        if (src) src->volume = _seVolume;
    }
}

void SoundManager::SetBGMVolume(float v) {
    _bgmVolume = ClampVolume(v);
    for (auto* src : _bgmSources) {
        if (src) src->volume = _bgmVolume;
    }
}

AudioSource* SoundManager::GetUnusedSESource() {
    for (auto* src : _seSources) {
        if (src && !src->IsPlaying()) return src;
    }
    return nullptr;
}

AudioSource* SoundManager::GetUnusedBGMSource() {
    for (auto* src : _bgmSources) {
        if (src && !src->IsPlaying()) return src;
    }
    return nullptr;
}

void SoundManager::LoadFolder(const std::string& folderPath, const std::string& keyPrefix, ClipMap& target, bool makeGroups) {
    std::string searchPath = folderPath + "/*";

    FILEINFO finfo;
    DWORD_PTR findHandle = FileRead_findFirst(searchPath.c_str(), &finfo);
    if (findHandle == static_cast<DWORD_PTR>(-1)) return;

    do {
        std::string name = finfo.Name;
        if (name == "." || name == "..") continue;

        std::string fullPath = folderPath + "/" + name;

        if (finfo.DirFlag) {
            // サブフォルダを再帰走査
            LoadFolder(fullPath, keyPrefix + name + "/", target, makeGroups);
            continue;
        }

        // 拡張子を抽出して小文字化
        auto dotPos = name.find_last_of('.');
        if (dotPos == std::string::npos) continue;
        std::string ext = name.substr(dotPos + 1);
        for (auto& c : ext) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (ext != "wav" && ext != "mp3" && ext != "ogg") continue;

        // フォルダ込みの拡張子なしパスをキーにする
        std::string stem = name.substr(0, dotPos);
        std::string key = keyPrefix + stem;

        if (target.find(key) != target.end()) continue;

        auto clip = std::make_unique<AudioClip>();
        if (!clip->Load(fullPath.c_str(), false)) continue;
        target.emplace(key, std::move(clip));

        // 末尾が _00 のような番号なら、番号を外した名前でもまとめて引けるようにする
        if (!makeGroups) continue;
        size_t length = stem.size();
        bool hasNumber = length > 3
            && stem[length - 3] == '_'
            && std::isdigit(static_cast<unsigned char>(stem[length - 2]))
            && std::isdigit(static_cast<unsigned char>(stem[length - 1]));
        if (hasNumber) {
            _seGroups[keyPrefix + stem.substr(0, length - 3)].push_back(key);
        }
    } while (FileRead_findNext(findHandle, &finfo) != -1);

    FileRead_findClose(findHandle);
}
