#include "TutorialSteps.h"

namespace {
    // ----- チュートリアルの段 上から順に出す -----
    // { 判定, 練習の相手, 回数, 見出し, { 説明 5 行まで } }
    // 並べ替えたり文字を書き換えたりするのはここだけでよい 判定の中身は TutorialDirector.cpp
    // 操作の説明は「キーボード / マウス」と「コントローラー」の 2 つだけ書く
    // J K L Q E のような、キーボードだけの別の押し方は書かない (PlayerController ではそのまま使える)
    const TutorialStep STEPS[] = {
        { TutorialGoal::Read, TutorialPartner::None, 1,
            "ようこそチュートリアルへ",
            { "敵を吹っ飛ばして場外へ飛ばすゲームです",
              "上に出る指示をやってみると、次へ進みます",
              "練習の間は、何をされても負けません" } },

        { TutorialGoal::Move, TutorialPartner::None, 1,
            "動いてみよう",
            { "キーボード / WASD",
              "コントローラー / 左スティック" } },

        { TutorialGoal::Look, TutorialPartner::None, 1,
            "見回してみよう",
            { "キーボード / マウス",
              "コントローラー / 右スティック で視点を回す",
              "キーボード / C",
              "コントローラー / L1 で、正面を向く" } },

        { TutorialGoal::Jump, TutorialPartner::None, 2,
            "ジャンプしてみよう",
            { "キーボード / SPACE",
              "コントローラー / A でジャンプ" } },

        { TutorialGoal::Combo, TutorialPartner::Dummy, 3,
            "練習台を斬ってみよう",
            { "キーボード / 左クリック",
              "コントローラー / X で斬る",
              "3 回続けてコンボにしよう" } },

        { TutorialGoal::Special, TutorialPartner::Dummy, 1,
            "必殺技で大勢の敵を倒そう",
            { "コンボを当てたり、敵を巻き込んで吹き飛ばしたりすると、バースターゲージが溜まる",
              "キーボード / F",
              "コントローラー / Y",
              "バースターゲージを溜めて、必殺技を放とう" } },

        { TutorialGoal::Charge, TutorialPartner::Dummy, 3,
            "溜め斬り",
            { "キーボード / 左右クリック",
              "コントローラー / R2 を押し続けて溜める",
              "3 段階まで溜めて、敵を吹っ飛ばそう" } },

        { TutorialGoal::AirSlam, TutorialPartner::Dummy, 1,
            "空中の技",
            { "キーボード / SPACE + 左クリック",
              "コントローラー / A + X",
              "空中で溜め斬り (真下へ叩きつける)",
              "キーボード / 空中で左右クリック",
              "コントローラー / 空中で R2" } },

        { TutorialGoal::Guard, TutorialPartner::Slasher, 2,
            "ガードしよう",
            { "キーボード / 右クリック",
              "コントローラー / L2",
              "敵の攻撃を正面から受けると、ガードできる",
              "ガードで 2 回受けてみよう" } },

        { TutorialGoal::Dodge, TutorialPartner::Slasher, 2,
            "回避しよう",
            { "キーボード / 右クリック + SPACE",
              "コントローラー / R1",
              "2 回回避してみよう" } },

        { TutorialGoal::JustDodge, TutorialPartner::Slasher, 2,
            "ジャスト回避",
            { "敵の頭の上が光った瞬間に回避すると、ジャスト回避",
              "敵がゆっくりになり、目の前へ寄って反撃できる",
              "2 回決めよう" } },

        { TutorialGoal::AvoidHeavy, TutorialPartner::Heavy, 2,
            "赤い攻撃に気をつけよう",
            { "頭の上が赤く光る攻撃は、ガードでは防げない",
              "赤い攻撃を 2 回かわそう" } },

        { TutorialGoal::OwnHeat, TutorialPartner::None, 1,
            "自分のバースト値に気をつけよう",
            { "攻撃を食らうと、自分のバースト値も溜まる",
              "バースト値が溜まると体が赤くなり、湯気が出る 気をつけよう",
              "値は全回復で 0 に戻る (EASY は 3、NORMAL は 4、HARD は 5 フェーズごと)" } },

        { TutorialGoal::HeatEnemy, TutorialPartner::Dummy, 1,
            "敵のバースト値を溜めよう",
            { "敵に体力はない 攻撃を当てるとバースト値が溜まり、遠くへ飛ぶようになる",
              "バースト値が溜まると、湯気が出て赤く光る",
              "練習台を赤くしよう" } },

        { TutorialGoal::BreakWall, TutorialPartner::Dummy, 1,
            "壁を割って倒そう",
            { "バースト値が溜まった敵を勢いよく壁にぶつけると、壁を割って場外へ飛ぶ これが倒し方",
              "溜め斬りなら、敵を大きく吹っ飛ばせる",
              "練習台で壁を割ろう" } },

        { TutorialGoal::DropBee, TutorialPartner::Bee, 1,
            "空の ビーザトール を落とそう",
            { "ビーザトール は空にいるので、地上の斬りは届かない",
              "対空斬り (SPACE + 左クリック / A + X) で落としてから叩く",
              "本番の ビーザトール は、離れたところから針を撃ってくる" } },

        { TutorialGoal::AvoidStomp, TutorialPartner::Golem, 1,
            "ロックマキナ の攻撃を回避しよう",
            { "地面に赤い輪が広がったら踏みつけ ガードできないので、輪の外へ出るか回避",
              "ロックマキナ は剣では吹き飛ばない ほかの敵を吹き飛ばしてぶつけると飛ぶ",
              "踏みつけを 1 回回避しよう" } },

        { TutorialGoal::Finish, TutorialPartner::None, 1,
            "準備完了",
            { "敵を全部倒すと次のフェーズへ フェーズ 10 を越えれば勝ち",
              "全回復は EASY 3 / NORMAL 4 / HARD 5 フェーズごと",
              "決定で本番へ" } },
    };

    constexpr int STEP_COUNT = static_cast<int>(sizeof(STEPS) / sizeof(STEPS[0]));
}

int TutorialSteps::GetCount() {
    return STEP_COUNT;
}

const TutorialStep& TutorialSteps::Get(int index) {
    if (index < 0) index = 0;
    if (index >= STEP_COUNT) index = STEP_COUNT - 1;
    return STEPS[index];
}