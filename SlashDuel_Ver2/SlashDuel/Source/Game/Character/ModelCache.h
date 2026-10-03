#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>

// 同じモデルを何体も出すときの元データ
// ファイルからは1回だけ読み、各キャラはそれを複製して使う
// 敵が十体並んでも読み込みは1回で済む
namespace ModelCache {

    inline int Get(const std::string& path) {
        static std::unordered_map<std::string, int> handles;

        auto it = handles.find(path);
        if (it != handles.end()) return it->second;

        // 読めなかったときも -1 を覚えておき、毎回読み直さない
        int handle = MV1LoadModel(path.c_str());
        handles.emplace(path, handle);
        return handle;
    }
}
