#include "MeshCollider.h"
#include "Renderer.h"

void MeshCollider::SetFromRenderer(Renderer* renderer) {
    if (renderer) {
        SetModelHandle(renderer->ModelHandle);
    }
}
