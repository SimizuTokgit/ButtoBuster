#include "RenderManager.h"
#include "Renderer.h"

void RenderManager::RenderAll() {
    // 半透明のものは不透明なものを全部描いてから描く
    // 先に描くと、あとから描いたモデルに上書きされて前後が崩れる
    // 同じ順番のもの同士は登録順を保つ
    std::stable_sort(_renderers.begin(), _renderers.end(), [](Renderer* a, Renderer* b) {
        return a->renderQueue < b->renderQueue;
    });

    for (auto* renderer : _renderers) {
        if (renderer) {
            renderer->Render();
        }
    }
}
