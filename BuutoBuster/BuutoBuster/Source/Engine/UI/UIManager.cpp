#include "UIManager.h"
#include "UIImage.h"

void UIManager::RenderAll() {
    // sortingOrder順にソート（小さい値が先に描画される）
    std::sort(_images.begin(), _images.end(), [](UIImage* a, UIImage* b) {
        return a->sortingOrder < b->sortingOrder;
    });

    for (auto* image : _images) {
        if (image && image->visible) {
            image->Render();
        }
    }
}
