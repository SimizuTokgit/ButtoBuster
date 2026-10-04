#include "SaveData.h"
#include <cstdio>

namespace {
    const char* const FILE_PATH = "SaveData.txt";
}

int SaveData::LoadBestPhase() {
    FILE* file = nullptr;
    if (fopen_s(&file, FILE_PATH, "r") != 0 || !file) return 0;

    int phase = 0;
    if (fscanf_s(file, "%d", &phase) != 1) phase = 0;
    fclose(file);

    // 壊れた値で最高記録が変にならないように
    return (phase > 0) ? phase : 0;
}

void SaveData::SaveBestPhase(int phase) {
    FILE* file = nullptr;
    if (fopen_s(&file, FILE_PATH, "w") != 0 || !file) return;

    fprintf(file, "%d\n", phase);
    fclose(file);
}
