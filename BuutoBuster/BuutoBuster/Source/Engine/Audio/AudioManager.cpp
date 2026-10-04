#include "AudioManager.h"
#include "AudioListener.h"
#include "AudioSource.h"

void AudioManager::Update() {
    // リスナーの3D位置を更新
    if (_activeListener) {
        _activeListener->UpdateListenerPosition();
    }

    // 全AudioSourceの3D位置を更新
    for (auto* source : _sources) {
        if (source) {
            source->UpdateSourcePosition();
        }
    }
}
