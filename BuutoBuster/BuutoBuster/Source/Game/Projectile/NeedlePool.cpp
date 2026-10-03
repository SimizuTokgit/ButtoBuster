#include "NeedlePool.h"
#include "Needle.h"
#include "ModelCache.h"
#include "GameObject.h"
#include "MeshRenderer.h"

NeedlePool::~NeedlePool() {
    if (_instance == this) _instance = nullptr;
}

void NeedlePool::Initialize(int count) {
    _instance = this;

    int source = ModelCache::Get("Data/Character/Needle/Needle.mv1");

    for (int i = 0; i < count; ++i) {
        auto* child = gameObject->AddChild("Needle");
        auto* needle = child->AddComponent<Needle>();
        auto* renderer = child->AddComponent<MeshRenderer>();
        renderer->LoadDuplicate(source);

        needle->SetRenderer(renderer);
        needle->Deactivate();
        _needles.push_back(needle);
    }
}

bool NeedlePool::Fire(VECTOR position, VECTOR direction, int damage) {
    for (auto* needle : _needles) {
        if (needle->IsFlying()) continue;

        needle->Launch(position, direction, damage);
        return true;
    }
    return false;
}

int NeedlePool::GetFlyingCount() const {
    int count = 0;
    for (const auto* needle : _needles) {
        if (needle->IsFlying()) count++;
    }
    return count;
}
