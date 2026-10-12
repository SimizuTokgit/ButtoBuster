#pragma once

// 操作の一覧 タイトルの画面と、HUD (操作の画像が読めないとき) が同じ表から描く
// ボタンを変えたら、PlayerController とこの表をそろえる
// 1 行は { 操作, キーボード・マウス / パッド, 補足 } 空の行は 1 行あける
namespace ControlsList {
    struct Row {
        const char* name;
        const char* keys;
        const char* note;
    };

    inline constexpr Row ROWS[] = {
        { "移動",              "WASD / 左スティック",        "" },
        { "視点",              "マウス, Q E / 右スティック", "" },
        { "正面を向く",        "C / L1",                     "" },
        { "攻撃",              "J, 左クリック / X",          "3 段までつながる" },
        { "ガード",            "K, 右クリック / L2",         "押している間 正面を守る" },
        { "ジャンプ",          "SPACE / A",                  "" },
        { "必殺技",            "F / Y",                      "バースターゲージが満タンのとき" },
        { "",                  "",                           "" },
        { "攻撃 + ジャンプ",   "対空斬り",                   "" },
        { "攻撃 + ガード",     "溜め斬り",                   "R2  押し続けて溜める" },
        { "ガード + ジャンプ", "回避",                       "R1  一瞬だけ無敵" },
    };

    inline constexpr int ROW_COUNT = static_cast<int>(sizeof(ROWS) / sizeof(ROWS[0]));
}
