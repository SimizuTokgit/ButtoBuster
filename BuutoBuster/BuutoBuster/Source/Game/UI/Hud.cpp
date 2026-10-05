#include "Hud.h"
#include "GameFont.h"
#include "Player.h"
#include "PhaseDirector.h"
#include "Enemy.h"
#include "CharacterRegistry.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

namespace {
    constexpr unsigned int WHITE = 0xFFFFFF;

    // ----- 回避の残りのバー (左上) -----
    // 前の体力バーと同じ場所と大きさ 続けて回避できる回数 (PlayerData の dodgeCount) に分け、1 つが 1 回分
    constexpr int DODGE_BAR_X = 40;
    constexpr int DODGE_BAR_Y = 34;
    constexpr int DODGE_BAR_WIDTH = 380;
    constexpr int DODGE_BAR_HEIGHT = 22;

    // 分けたバーの間の隙間
    constexpr int DODGE_BAR_GAP = 8;

    // 使える分の色と、戻っている途中の色 溜まりきると使える色に変わる
    constexpr unsigned int DODGE_READY_COLOR = 0x78E6FF;
    constexpr unsigned int DODGE_CHARGING_COLOR = 0x3A7088;

    // バーの後ろに敷く黒の濃さ 0〜255 3D の上でも読みやすくする
    constexpr int DODGE_BAR_SHADE_ALPHA = 170;

    // バーの下に出す名前
    constexpr const char* DODGE_BAR_LABEL = "回避";

    // 頭の上の画面の位置 カメラの後ろにあるなら false
    bool GetHeadScreenPosition(const Character& character, VECTOR& outScreen) {
        VECTOR head = VAdd(character.GetCenter(), VGet(0.0f, character.bodyHeight * 0.5f + 40.0f, 0.0f));
        outScreen = ConvWorldPosToScreenPos(head);
        return outScreen.z > 0.0f && outScreen.z < 1.0f;
    }
}

void Hud::Setup(Player* player, PhaseDirector* director) {
    _player = player;
    _director = director;
}

void Hud::Render() {
    if (!_player || !_director) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    if (isStateVisible) DrawStates();

    DrawDanger(screenWidth, screenHeight);
    DrawDodgeStock();
    DrawPhaseInfo(screenWidth, screenHeight);
    DrawCombo(screenWidth, screenHeight);
    DrawControls(screenWidth, screenHeight);
}

void Hud::DrawDanger(int screenWidth, int screenHeight) {
    // 吹っ飛ばされ値が許容値に届いたら、画面の縁を赤く脈打たせる 今壁に飛ばされたら負けると気づけるように
    if (_player->IsDead() || _player->GetBlowRatio() < 1.0f) return;

    float pulse = (sinf(GetNowCount() / 1000.0f * 6.0f) + 1.0f) * 0.5f;
    constexpr int EDGE = 18;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(40 + pulse * 70));
    DrawBox(0, 0, screenWidth, EDGE, 0xC02020, TRUE);
    DrawBox(0, screenHeight - EDGE, screenWidth, screenHeight, 0xC02020, TRUE);
    DrawBox(0, 0, EDGE, screenHeight, 0xC02020, TRUE);
    DrawBox(screenWidth - EDGE, 0, screenWidth, screenHeight, 0xC02020, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Hud::DrawDodgeStock() {
    int count = _player->data.dodgeCount;
    if (count <= 0) return;

    int segmentWidth = (DODGE_BAR_WIDTH - DODGE_BAR_GAP * (count - 1)) / count;
    int bottom = DODGE_BAR_Y + DODGE_BAR_HEIGHT;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, DODGE_BAR_SHADE_ALPHA);
    DrawBox(DODGE_BAR_X - 4, DODGE_BAR_Y - 4, DODGE_BAR_X + DODGE_BAR_WIDTH + 4, bottom + 4, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 左から順に溜まる 戻っている途中の 1 つは、溜まった分だけ伸びる
    float stock = _player->GetDodgeStock();
    for (int i = 0; i < count; ++i) {
        int left = DODGE_BAR_X + (segmentWidth + DODGE_BAR_GAP) * i;

        float fill = stock - static_cast<float>(i);
        if (fill > 1.0f) fill = 1.0f;
        if (fill > 0.0f) {
            unsigned int color = (fill >= 1.0f) ? DODGE_READY_COLOR : DODGE_CHARGING_COLOR;
            DrawBox(left, DODGE_BAR_Y, left + static_cast<int>(segmentWidth * fill), bottom, color, TRUE);
        }
        DrawBox(left, DODGE_BAR_Y, left + segmentWidth, bottom, WHITE, FALSE);
    }

    GameFont::Draw(DODGE_BAR_X, bottom + 6, DODGE_BAR_LABEL, WHITE, GameFont::Size::Small);
}

void Hud::DrawPhaseInfo(int screenWidth, int screenHeight) {
    int right = screenWidth - 40;
    char text[64];

    snprintf(text, sizeof(text), "PHASE %d", _director->GetPhase());
    GameFont::DrawRight(right, 20, text, WHITE, GameFont::Size::Large);

    snprintf(text, sizeof(text), "残り %d", _director->GetRemainingEnemyCount());
    GameFont::DrawRight(right, 84, text, 0xE0E0E0, GameFont::Size::Medium);

    int untilHeal = _director->GetPhasesUntilHeal();
    if (untilHeal == 0) {
        snprintf(text, sizeof(text), "このフェーズを越えれば全回復");
    }
    else {
        snprintf(text, sizeof(text), "全回復まで あと %d フェーズ", untilHeal);
    }
    GameFont::DrawRight(right, 122, text, 0x90F0B0, GameFont::Size::Small);

    snprintf(text, sizeof(text), "撃破 %d", _director->GetKillCount());
    GameFont::DrawRight(right, 148, text, 0xC0C0C0, GameFont::Size::Small);
}

void Hud::DrawCombo(int screenWidth, int screenHeight) {
    int combo = _player->GetCombo();
    if (combo < 2) return;

    // 途切れる直前に薄くして、あと少しで切れることを知らせる
    float alpha = _player->GetComboTimer() / 0.5f;
    if (alpha > 1.0f) alpha = 1.0f;

    char text[16];
    snprintf(text, sizeof(text), "%d", combo);

    int right = screenWidth - 130;
    int top = screenHeight / 2 - 80;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
    GameFont::DrawRight(right, top, text, 0xFFD060, GameFont::Size::Huge);
    GameFont::Draw(right + 8, top + 52, "HIT", 0xFFD060, GameFont::Size::Medium);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Hud::DrawControls(int screenWidth, int screenHeight) {
    const char* lines[] = {
        "移動 WASD / 左スティック    視点 マウス / Q E / 右スティック    ロックオン L / ホイール押し / LT",
        "攻撃 左クリック / X    ガード 右クリック / B    ジャンプ SPACE / A",
        "ガードを握ったまま 攻撃で強斬り ジャンプで回避    攻撃+ジャンプ 対空斬り",
    };
    constexpr int LINE_COUNT = sizeof(lines) / sizeof(lines[0]);

    int lineHeight = GameFont::GetHeight(GameFont::Size::Small) + 6;
    int top = screenHeight - 24 - lineHeight * LINE_COUNT;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
    for (int i = 0; i < LINE_COUNT; ++i) {
        GameFont::Draw(40, top + lineHeight * i, lines[i], 0xE8E8E8, GameFont::Size::Small);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Hud::DrawStates() {
    for (const Character* character : CharacterRegistry::GetAll()) {
        if (!character) continue;

        VECTOR screen;
        if (!GetHeadScreenPosition(*character, screen)) continue;

        // 攻撃の番を持っている敵は赤く出す 取り巻きの動きを確かめるため
        unsigned int color = 0x80FFFF;
        if (const auto* enemy = dynamic_cast<const Enemy*>(character)) {
            if (_director->GetTokens().Has(enemy)) color = 0xFF6060;
        }

        // 吹っ飛ばされ値は普段は数字で出さないので、調整するときはここで見る
        // 隙があるときは、あと何秒動けないかも出す 敵が隙を狙う AI を作るときに見る
        char label[96];
        float opening = character->GetOpeningTime();
        if (opening > 0.0f) {
            snprintf(label, sizeof(label), "%s  %.0f / %.0f  隙 %.2f", character->GetStateName(),
                character->GetBlowValue(), character->GetBlowLimit(), opening);
        }
        else {
            snprintf(label, sizeof(label), "%s  %.0f / %.0f", character->GetStateName(),
                character->GetBlowValue(), character->GetBlowLimit());
        }
        GameFont::DrawCentered(static_cast<int>(screen.x), static_cast<int>(screen.y) - 26,
            label, color, GameFont::Size::Small);
    }
}
