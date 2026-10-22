#pragma once
#include <vector>
#include "ContainerUtil.h"

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
        ContainerUtil::AddUnique(_systems, system);
    }

    /// <summary>
    /// ParticleSystemの登録解除
    /// </summary>
    void Unregister(ParticleSystem* system) {
        ContainerUtil::Remove(_systems, system);
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
