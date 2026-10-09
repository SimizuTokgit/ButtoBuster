#include "Hud.h"
#include "GameFont.h"
#include "Player.h"
#include "PhaseDirector.h"
#include "Enemy.h"
#include "CharacterRegistry.h"
#include "SoundManager.h"
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

    // ----- 吹っ飛ばされそうな危なさ (画面の縁の赤み) -----
    // 自分の吹っ飛ばされ値が許容値のこの割合を越えるごとに、縁の赤みを濃く太く、脈を速くする
    // あとどのくらいで壁を割られてしまうかを、数字を出さずに分かるように
    struct DangerLook {
        float ratio;            // この割合から (1 で許容値に届いた 今壁に飛ばされたら負け)
        int width;              // 縁の太さ px
        int alphaMin;           // 脈の弱いときの濃さ 0〜255
        int alphaMax;           // 脈の強いときの濃さ 0〜255
        float beatsPerSecond;   // 1 秒に脈打つ回数
    };
    const DangerLook DANGER_LOOKS[] = {
        { 0.5f,  30, 10,  45, 1.0f },   // 半分を越えた うっすら、ゆっくり
        { 0.75f, 45, 30,  90, 1.6f },   // 4 分の 3 を越えた はっきり
        { 1.0f,  64, 60, 150, 2.4f },   // 許容値に届いた 濃く、速く
    };
    constexpr int DANGER_LOOK_COUNT = sizeof(DANGER_LOOKS) / sizeof(DANGER_LOOKS[0]);

    constexpr unsigned int DANGER_COLOR = 0xC02020;

    // 縁をこの数の帯に分け、内側ほど薄くしてぼかす
    constexpr int DANGER_BANDS = 5;

    // 許容値に届いている間、脈ごとに鳴らす音 空なら鳴らさない (心音の音を Data/Sound/SE に入れたら名前を書く)
    constexpr const char* DANGER_BEAT_SOUND = "";
    constexpr float DANGER_BEAT_VOLUME = 0.8f;

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

    // チュートリアルの間はフェーズの表示を出さない
    if (_director->GetStep() != PhaseDirector::Step::Practice) DrawPhaseInfo(screenWidth, screenHeight);

    DrawCombo(screenWidth, screenHeight);
    DrawControls(screenWidth, screenHeight);
}

void Hud::DrawDanger(int screenWidth, int screenHeight) {
    // 吹っ飛ばされ値が溜まるほど、画面の縁を赤く脈打たせる 段の数値は先頭の DANGER_LOOKS
    if (_player->IsDead()) return;

    // 今の吹っ飛ばされ値で届いている、いちばん強い段を選ぶ
    float ratio = _player->GetBlowRatio();
    int stage = -1;
    for (int i = 0; i < DANGER_LOOK_COUNT; ++i) {
        if (ratio >= DANGER_LOOKS[i].ratio) stage = i;
    }
    if (stage < 0) return;
    const DangerLook& look = DANGER_LOOKS[stage];

    // 0〜1 で脈打つ
    float seconds = GetNowCount() / 1000.0f;
    float pulse = (sinf(seconds * look.beatsPerSecond * DX_TWO_PI_F) + 1.0f) * 0.5f;
    float alpha = look.alphaMin + (look.alphaMax - look.alphaMin) * pulse;

    // 外側から内側へ、帯ごとに薄くしてぼかす
    int band = look.width / DANGER_BANDS;
    for (int i = 0; i < DANGER_BANDS; ++i) {
        float fade = 1.0f - static_cast<float>(i) / DANGER_BANDS;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * fade));

        int inner = band * i;
        int outer = band * (i + 1);
        DrawBox(0, inner, screenWidth, outer, DANGER_COLOR, TRUE);                                          // 上
        DrawBox(0, screenHeight - outer, screenWidth, screenHeight - inner, DANGER_COLOR, TRUE);            // 下
        DrawBox(inner, outer, outer, screenHeight - outer, DANGER_COLOR, TRUE);                             // 左
        DrawBox(screenWidth - outer, outer, screenWidth - inner, screenHeight - outer, DANGER_COLOR, TRUE); // 右
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // いちばん強い段では、脈ごとに音を鳴らす
    bool isMaxStage = stage == DANGER_LOOK_COUNT - 1;
    if (isMaxStage && DANGER_BEAT_SOUND[0] != '\0') {
        int beat = static_cast<int>(seconds * look.beatsPerSecond);
        if (beat != _lastDangerBeat) {
            _lastDangerBeat = beat;
            SoundManager::Instance().PlaySE(DANGER_BEAT_SOUND, DANGER_BEAT_VOLUME);
        }
    }
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

    // 全回復より先に勝ちが来るなら、勝ちまでを出す 最後のフェーズを越えたときは全回復しないので
    int untilHeal = _director->GetPhasesUntilHeal();
    int untilVictory = _director->GetPhasesUntilVictory();
    unsigned int goalColor = 0x90F0B0;
    if (untilVictory >= 0 && untilVictory <= untilHeal) {
        goalColor = 0xFFD060;
        if (untilVictory == 0) {
            snprintf(text, sizeof(text), "このフェーズを越えれば勝利");
        }
        else {
            snprintf(text, sizeof(text), "勝利まで あと %d フェーズ", untilVictory);
        }
    }
    else if (untilHeal == 0) {
        snprintf(text, sizeof(text), "このフェーズを越えれば全回復");
    }
    else {
        snprintf(text, sizeof(text), "全回復まで あと %d フェーズ", untilHeal);
    }
    GameFont::DrawRight(right, 122, text, goalColor, GameFont::Size::Small);

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
        "移動 WASD / 左スティック    視点 マウス / Q E / 右スティック    視点を戻す L / ホイール押し / R3",
        "攻撃 左クリック / X    ガード 右クリック / L2    ジャンプ SPACE / A",
        "溜め斬り 攻撃+ガード / R2    回避 ガード+ジャンプ / L1    対空斬り 攻撃+ジャンプ / X+A",
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
        // 行動の木で動く敵は、頭が今していること (葉の名前) も体の状態の横に出す
        unsigned int color = 0x80FFFF;
        char state[64];
        snprintf(state, sizeof(state), "%s", character->GetStateName());
        if (const auto* enemy = dynamic_cast<const Enemy*>(character)) {
            if (_director->GetTokens().Has(enemy)) color = 0xFF6060;
            if (enemy->GetThinking()[0] != '\0') {
                snprintf(state, sizeof(state), "%s / %s", character->GetStateName(), enemy->GetThinking());
            }
        }

        // 吹っ飛ばされ値は普段は数字で出さないので、調整するときはここで見る
        // 隙があるときは、あと何秒動けないかも出す 敵が隙を狙う AI を作るときに見る
        char label[128];
        float opening = character->GetOpeningTime();
        if (opening > 0.0f) {
            snprintf(label, sizeof(label), "%s  %.0f / %.0f  隙 %.2f", state,
                character->GetBlowValue(), character->GetBlowLimit(), opening);
        }
        else {
            snprintf(label, sizeof(label), "%s  %.0f / %.0f", state,
                character->GetBlowValue(), character->GetBlowLimit());
        }
        GameFont::DrawCentered(static_cast<int>(screen.x), static_cast<int>(screen.y) - 26,
            label, color, GameFont::Size::Small);
    }
}
