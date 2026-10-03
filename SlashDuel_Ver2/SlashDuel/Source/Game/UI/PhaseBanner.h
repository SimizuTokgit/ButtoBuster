#pragma once
#include "UIImage.h"

class PhaseDirector;

// フェーズの区切りを大きく出す
// 開始 クリア 全回復
// 新しい種類の敵が出始めるフェーズでは、その名前も出す
class PhaseBanner : public UIImage {
private:
    PhaseDirector* _director = nullptr;

public:
    void Setup(PhaseDirector* director);
    void Render() override;

private:
    const char* FindNewEnemyName(int phase) const;
};
