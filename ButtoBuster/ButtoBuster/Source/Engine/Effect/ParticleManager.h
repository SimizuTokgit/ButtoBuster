#pragma once
#include <vector>
#include <algorithm>

class ParticleSystem;

/// <summary>
/// ParticleSystem管理シングルトンクラス
/// 登録されたParticleSystemを一括でUpdate・Renderする
/// </summary>
class ParticleManager {
private:
    std::vector<ParticleSystem*> _systems;

public:
    /// <summary>シングルトンインスタンスを取得</summary>
    static ParticleManager& Instance() {
        static ParticleManager instance;
        return instance;
    }

    // コピー禁止
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    /// <summary>
    /// ParticleSystemを登録
    /// </summary>
    void Register(ParticleSystem* system) {
        if (system) {
            auto it = std::find(_systems.begin(), _systems.end(), system);
            if (it == _systems.end()) {
                _systems.push_back(system);
            }
        }
    }

    /// <summary>
    /// ParticleSystemの登録解除
    /// </summary>
    void Unregister(ParticleSystem* system) {
        auto it = std::find(_systems.begin(), _systems.end(), system);
        if (it != _systems.end()) {
            _systems.erase(it);
        }
    }

    /// <summary>
    /// 登録されている全ParticleSystemを更新
    /// </summary>
    void UpdateAll(float deltaTime);

    /// <summary>
    /// 登録されている全ParticleSystemを描画
    /// </summary>
    void RenderAll();

    /// <summary>
    /// 登録されているParticleSystem数を取得
    /// </summary>
    size_t GetSystemCount() const { return _systems.size(); }

    /// <summary>
    /// 全ParticleSystemをクリア（シーン切り替え時など）
    /// </summary>
    void Clear() { _systems.clear(); }

private:
    ParticleManager() = default;
    ~ParticleManager() = default;
};
