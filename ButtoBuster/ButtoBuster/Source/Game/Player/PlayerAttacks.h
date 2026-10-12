#pragma once
#include "AttackData.h"

struct PlayerData;

// プレイヤーの技の数値
// 判定の出る時間は、アニメで剣を振っているフレームに合わせてある
namespace PlayerAttacks {
    constexpr int SLASH_COUNT = 3;

    // 振るときの声 コンボの段ごと 空中の斬りも同じ 叩きつけは締め (3 段目) と同じ声
    // 声の大きさは PlayerData::VOICE_VOLUME
    constexpr const char* SLASH_VOICES[SLASH_COUNT] = {
        "Player/Vc_Attack1", "Player/Vc_Attack2", "Player/Vc_Attack3",
    };

    // 通常の斬り 0〜2 段目
    const AttackData& GetSlash(int index);
    const AttackData& GetStrong();
    const AttackData& GetAntiAir();

    // ヘビーアタックの溜めの段階 0 は溜めずに離したとき
    constexpr int CHARGE_LEVEL_MAX = 3;
    const AttackData& GetHeavy(int level);

    // 空中の斬り 0〜2 段目 地上と同じ振りで、1 2 段目は相手を浮かせ、3 段目で吹き飛ばす
    const AttackData& GetAirSlash(int index);

    // 空中から真下への叩きつけ
    // 落ちる途中で、この半径にいる宙の敵を捕まえて地面へ叩き落とす (GetAirSlamCatch) 敵は地面で跳ね上がる
    // 着地したら、前の扇形の敵を吹き飛ばす (GetAirSlam)
    // 半径は空中の斬りが届く距離と同じくらいにして、浮かせた敵を取りこぼさないようにする
    constexpr float AIR_SLAM_CATCH_RADIUS = 220.0f;
    const AttackData& GetAirSlam();
    const AttackData& GetAirSlamCatch();

    // 必殺技の雷 1 発分 全部の敵に、プレイヤーから外へ向けて当てる
    const AttackData& GetSpecial();

    // PlayerData の倍率 (強化で上がる分) を掛けた技にする 技を振り始めるときに通す
    // staleCount は直近に同じ技を振った回数 (Player::UseMove) その分だけ威力を下げ、あと隙を伸ばす (OP 相殺)
    AttackData ApplyRates(const AttackData& base, const PlayerData& playerData, int staleCount = 0);

    // OP 相殺で、あと隙を元の何倍にするか あと隙を AttackData の外に持つ技 (叩きつけ) が使う
    float GetStaleRecoveryRate(int staleCount, const PlayerData& playerData);

    // ジャスト回避のあとの反撃の締め 元の技を、必ず吹き飛ばす重い一撃にする どれだけ重くするかは PlayerData
    AttackData CreateCounter(const AttackData& base, const PlayerData& playerData);

    // ジャスト回避のあとの反撃のうち、締めの前の斬り のけぞりと浮かせはそのままで、ダメージだけ重くする
    // 吹き飛ばしの技にはしない ただし重くした分で許容値に届いた相手は、いつもどおり吹き飛ぶ
    AttackData CreateCounterCombo(const AttackData& base, const PlayerData& playerData);
}
