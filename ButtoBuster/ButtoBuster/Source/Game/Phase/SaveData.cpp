#include "SaveData.h"
#include <cstdio>

namespace {
    const char* const FILE_PATH = "SaveData.txt";

    constexpr int COUNT = static_cast<int>(Difficulty::Count);

    // 全部の難易度の記録を読む 無い分は 0
    void LoadAll(int (&phases)[COUNT]) {
        for (int& phase : phases) phase = 0;

        FILE* file = nullptr;
        if (fopen_s(&file, FILE_PATH, "r") != 0 || !file) return;

        int values[COUNT] = {};
        int readCount = 0;
        while (readCount < COUNT && fscanf_s(file, "%d", &values[readCount]) == 1) readCount++;
        fclose(file);

        if (readCount == 1) {
            // 難易度を作る前のファイル 1 つだけの記録はノーマルのもの
            phases[static_cast<int>(Difficulty::Normal)] = values[0];
        }
        else {
            for (int i = 0; i < readCount; ++i) phases[i] = values[i];
        }

        // 壊れた値で最高記録が変にならないように
        for (int& phase : phases) {
            if (phase < 0) phase = 0;
        }
    }
}

int SaveData::LoadBestPhase(Difficulty difficulty) {
    int index = static_cast<int>(difficulty);
    if (index < 0 || index >= COUNT) return 0;

    int phases[COUNT];
    LoadAll(phases);
    return phases[index];
}

void SaveData::SaveBestPhase(Difficulty difficulty, int phase) {
    int index = static_cast<int>(difficulty);
    if (index < 0 || index >= COUNT) return;

    // ほかの難易度の記録を消さないよう、全部読んでから 1 つだけ書き換える
    int phases[COUNT];
    LoadAll(phases);
    phases[index] = phase;

    FILE* file = nullptr;
    if (fopen_s(&file, FILE_PATH, "w") != 0 || !file) return;

    for (int value : phases) fprintf(file, "%d\n", value);
    fclose(file);
}
