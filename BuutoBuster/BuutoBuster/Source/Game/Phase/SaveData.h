#pragma once

// 最高記録の保存
// 到達フェーズ1つだけなので、テキストに数字を1行書くだけにしてある
namespace SaveData {
    int LoadBestPhase();
    void SaveBestPhase(int phase);
}
