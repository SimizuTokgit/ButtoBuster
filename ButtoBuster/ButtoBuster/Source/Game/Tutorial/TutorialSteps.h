#pragma once

// チュートリアルの 1 段で、何をしたら次へ進むか
// 段の順番と文字は TutorialSteps.cpp の表、できたかどうかの判定は TutorialDirector.cpp
enum class TutorialGoal {
    Read,           // 読むだけ 動けず、決定で次へ
    Move,           // 歩き回る
    Look,           // 視点を回す
    Jump,           // ジャンプする (count 回)
    Combo,          // 続けて当てる (count 回)
    Charge,         // 溜め斬りを count 段階以上溜めて振る
    AirSlam,        // 空中から叩きつける (count 回)
    Guard,          // ガードで受ける (count 回)
    Dodge,          // 回避する (count 回)
    AvoidHeavy,     // 大振りを食らわずにやり過ごす (count 回)
    JustDodge,      // ジャスト回避を決める (count 回)
    OwnHeat,        // 自分の吹っ飛ばされ値を、わざと許容値まで溜めて見せる 読むだけ
    HeatEnemy,      // 練習の相手を許容値まで溜める
    BreakWall,      // 壁を割って敵を倒す (count 回)
    DropBee,        // 空の Bee を落とす (count 回)
    AvoidStomp,     // 踏みつけを食らわずにやり過ごす (count 回)
    Finish,         // 最後 決定で本番へ
};

// その段で出しておく練習の相手 数値は TutorialDirector.cpp の先頭
// 前の段と同じ相手なら出し直さず、そのまま使う
enum class TutorialPartner {
    None,
    Dummy,      // 練習台の Goblin 攻撃してこない
    Slasher,    // 普通の斬りだけを振る Goblin
    Heavy,      // 大振りだけを振る Goblin
    Bee,        // 飛んでいるだけの Bee 攻撃してこない
    Golem,      // 踏みつけだけを出す Golem
};

struct TutorialStep {
    TutorialGoal goal = TutorialGoal::Read;
    TutorialPartner partner = TutorialPartner::None;

    // 何回やったら次へ進むか 回数を数えない段では使わない
    int count = 1;

    // 大きく出す見出しと、その下の説明 説明は 3 行まで 使わない行は空のまま
    const char* title = "";
    const char* lines[3] = {};
};

namespace TutorialSteps {
    int GetCount();
    const TutorialStep& Get(int index);
}
