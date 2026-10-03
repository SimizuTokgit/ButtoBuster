#pragma once
#include <algorithm>
#include <vector>

class Character;

// 場に出ているキャラの一覧
// 攻撃の当たり判定やAIが相手を探すときに使う
// 同時に出るのは十数体なので、全員を順に見るだけで足りる
class CharacterRegistry {
private:
    static inline std::vector<Character*> _characters;

public:
    static void Add(Character* character) {
        if (!character) return;
        if (std::find(_characters.begin(), _characters.end(), character) != _characters.end()) return;
        _characters.push_back(character);
    }

    static void Remove(Character* character) {
        auto it = std::find(_characters.begin(), _characters.end(), character);
        if (it != _characters.end()) _characters.erase(it);
    }

    static const std::vector<Character*>& GetAll() { return _characters; }

    // まだ場にいるか 中身は見ずにアドレスだけで確かめる
    // 消えたかもしれない相手を持ち続けるとき、触る前に使う
    static bool Contains(const Character* character) {
        return std::find(_characters.begin(), _characters.end(), character) != _characters.end();
    }
};
