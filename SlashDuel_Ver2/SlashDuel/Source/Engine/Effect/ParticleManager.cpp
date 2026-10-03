#include "ParticleManager.h"
#include "ParticleSystem.h"

void ParticleManager::UpdateAll(float deltaTime) {
    // 粒が全部消えたシステムは Update の中で自分を一覧から外す
    // 同じ一覧を回したまま消すと、要素がずれて飛ばしたり二重に回したりするので控えてから回す
    std::vector<ParticleSystem*> targets = _systems;
    for (auto* system : targets) {
        if (system) {
            system->Update(deltaTime);
        }
    }
}

void ParticleManager::RenderAll() {
    for (auto* system : _systems) {
        if (system) {
            system->Render();
        }
    }
}
