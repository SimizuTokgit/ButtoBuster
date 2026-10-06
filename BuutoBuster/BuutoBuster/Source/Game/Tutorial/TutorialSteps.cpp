#include "TutorialSteps.h"

namespace {
    // ----- チュートリアルの段 上から順に出す -----
    // { 判定, 練習の相手, 回数, 見出し, { 説明 3 行まで } }
    // 並べ替えたり文字を書き換えたりするのはここだけでよい 判定の中身は TutorialDirector.cpp
    const TutorialStep STEPS[] = {
        { TutorialGoal::Read, TutorialPartner::None, 1,
            "ようこそ BUUTO BUSTER へ",
            { "敵を吹っ飛ばして壁にぶつけ、壁を割って場外へ飛ばすゲームです",
              "上に出る指示をやってみると、次へ進みます",
              "練習の間は、何をされても負けません" } },

        { TutorialGoal::Move, TutorialPartner::None, 1,
            "動いてみよう",
            { "WASD / 左スティック で動く",
              "奥へ倒すと、カメラが向いている先へ進む" } },

        { TutorialGoal::Look, TutorialPartner::None, 1,
            "見回してみよう",
            { "マウス / Q E / 右スティック で視点を回す",
              "L / ホイール押し / R3 で、背中側へ戻せる" } },

        { TutorialGoal::Jump, TutorialPartner::None, 2,
            "ジャンプしてみよう",
            { "SPACE / A でジャンプ" } },

        { TutorialGoal::Combo, TutorialPartner::Dummy, 3,
            "練習台を斬ってみよう",
            { "J / 左クリック / X で斬る 続けて押すと 3 段まで繋がる",
              "3 回続けて当てよう" } },

        { TutorialGoal::Charge, TutorialPartner::Dummy, 1,
            "溜め斬り",
            { "攻撃 + ガード (J + K / 左右クリック) か R2 を押し続けて溜め、離して振る",
              "光るたびに段階が上がり、遠くへ吹き飛ばせる そのぶん振ったあとの隙が大きい",
              "1 段階以上溜めて振ろう" } },

        { TutorialGoal::AirSlam, TutorialPartner::Dummy, 1,
            "空中の技",
            { "ジャンプ中に攻撃で、空中の斬り",
              "ジャンプ中に溜め斬りの操作で、真下へ叩きつける",
              "叩きつけてみよう" } },

        { TutorialGoal::Guard, TutorialPartner::Slasher, 2,
            "ガードしよう",
            { "K / 右クリック / L2 を押している間、正面からの攻撃を防ぐ",
              "敵の頭の上がオレンジに光ったら、普通の斬りが来る",
              "ガードで 2 回受けよう" } },

        { TutorialGoal::Dodge, TutorialPartner::Slasher, 2,
            "回避しよう",
            { "ガード + ジャンプ (K + SPACE) か L1 で回避 かわし始めは一瞬だけ無敵",
              "続けて 2 回まで 使った分は左上のバーで少しずつ戻る",
              "2 回回避しよう" } },

        { TutorialGoal::AvoidHeavy, TutorialPartner::Heavy, 2,
            "赤い合図は大振り",
            { "頭の上が赤く大きく光ったら大振り ガードできず、当たると吹き飛ばされる",
              "見たら回避するか、離れてかわそう",
              "2 回かわそう" } },

        { TutorialGoal::JustDodge, TutorialPartner::Slasher, 1,
            "ジャスト回避",
            { "攻撃が当たる直前に回避すると、ジャスト回避",
              "敵がゆっくりになり、目の前へ寄って反撃できる",
              "1 回決めよう" } },

        { TutorialGoal::OwnHeat, TutorialPartner::None, 1,
            "自分のホカホカに気をつけよう",
            { "攻撃を食らうと、自分の吹っ飛ばされ値も溜まる (今、わざと許容値まで溜めた)",
              "湯気が出て画面の縁が赤く脈打つ間に、壁へ飛ばされると負け (練習中は負けない)",
              "値は 5 フェーズごとの全回復で 0 に戻る" } },

        { TutorialGoal::HeatEnemy, TutorialPartner::Dummy, 1,
            "敵をホカホカにしよう",
            { "敵に体力はない 攻撃を当てると吹っ飛ばされ値が溜まり、遠くへ飛ぶようになる",
              "許容値に届くと、湯気が出て赤く光る (ホカホカ)",
              "練習台をホカホカにしよう" } },

        { TutorialGoal::BreakWall, TutorialPartner::Dummy, 1,
            "壁を割って倒そう",
            { "ホカホカの敵を勢いよく壁にぶつけると、壁を割って場外へ飛ぶ これが倒し方",
              "溜め斬りがいちばん遠くへ飛ぶ 冷めたら、また斬ってホカホカに",
              "練習台で壁を割ろう" } },

        { TutorialGoal::DropBee, TutorialPartner::Bee, 1,
            "空の Bee を落とそう",
            { "Bee は空にいるので、地上の斬りは届かない",
              "攻撃 + ジャンプ (J + SPACE / X + A) の対空斬りで落としてから叩く",
              "本番の Bee は、離れたところから針を撃ってくる" } },

        { TutorialGoal::AvoidStomp, TutorialPartner::Golem, 1,
            "Golem の踏みつけ",
            { "地面に赤い輪が広がったら踏みつけ ガードできないので、輪の外へ出るか回避",
              "Golem は剣では吹き飛ばない ほかの敵を吹き飛ばしてぶつけると飛ぶ",
              "踏みつけを 1 回かわそう" } },

        { TutorialGoal::Finish, TutorialPartner::None, 1,
            "準備完了",
            { "敵を全部倒すと次のフェーズへ フェーズ 10 を越えれば勝ち",
              "5 フェーズごとに全回復する",
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
