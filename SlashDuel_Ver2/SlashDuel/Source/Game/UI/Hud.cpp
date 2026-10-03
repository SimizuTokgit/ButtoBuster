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

    // 3D の上に重ねる背景 文字を読みやすくする
    void DrawShade(int left, int top, int right, int bottom, int alpha) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(left, top, right, bottom, 0x000000, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

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

void Hud::Update(float deltaTime) {
    if (!_player) return;

    // 減った分はすぐ消さず、白いバーで少し残す どれだけ食らったかが見える
    float ratio = _player->GetHpRatio();
    if (ratio >= _ghostRatio) {
        _ghostRatio = ratio;
    }
    else {
        _ghostRatio -= GHOST_SPEED * deltaTime;
        if (_ghostRatio < ratio) _ghostRatio = ratio;
    }
}

void Hud::Render() {
    if (!_player || !_director) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    DrawEnemyBars();
    if (isStateVisible) DrawStates();

    DrawPlayerHp(screenWidth, screenHeight);
    DrawPhaseInfo(screenWidth, screenHeight);
    DrawCombo(screenWidth, screenHeight);
    DrawControls(screenWidth, screenHeight);
}

void Hud::DrawPlayerHp(int screenWidth, int screenHeight) {
    constexpr int X = 40;
    constexpr int Y = 34;
    constexpr int WIDTH = 380;
    constexpr int HEIGHT = 22;

    float ratio = _player->GetHpRatio();

    // 残りが少ないときは画面の縁を赤く脈打たせる 体力を見ていなくても気づけるように
    if (ratio > 0.0f && ratio < 0.25f) {
        float pulse = (sinf(GetNowCount() / 1000.0f * 6.0f) + 1.0f) * 0.5f;
        constexpr int EDGE = 18;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(40 + pulse * 70));
        DrawBox(0, 0, screenWidth, EDGE, 0xC02020, TRUE);
        DrawBox(0, screenHeight - EDGE, screenWidth, screenHeight, 0xC02020, TRUE);
        DrawBox(0, 0, EDGE, screenHeight, 0xC02020, TRUE);
        DrawBox(screenWidth - EDGE, 0, screenWidth, screenHeight, 0xC02020, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    DrawShade(X - 4, Y - 4, X + WIDTH + 4, Y + HEIGHT + 4, 170);
    DrawBox(X, Y, X + static_cast<int>(WIDTH * _ghostRatio), Y + HEIGHT, 0xF0F0F0, TRUE);

    unsigned int color = 0x3CD070;
    if (ratio <= 0.25f) color = 0xE04848;
    else if (ratio <= 0.5f) color = 0xE8C040;
    DrawBox(X, Y, X + static_cast<int>(WIDTH * ratio), Y + HEIGHT, color, TRUE);
    DrawBox(X, Y, X + WIDTH, Y + HEIGHT, WHITE, FALSE);

    char text[32];
    snprintf(text, sizeof(text), "HP %d / %d", _player->hp, _player->maxHp);
    GameFont::Draw(X, Y + HEIGHT + 6, text, WHITE, GameFont::Size::Small);
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

void Hud::DrawEnemyBars() {
    for (const Enemy* enemy : _director->GetEnemies()) {
        if (enemy->IsDead()) continue;

        float timer = enemy->GetHpBarTimer();
        if (timer <= 0.0f) continue;

        VECTOR screen;
        if (!GetHeadScreenPosition(*enemy, screen)) continue;

        bool isLarge = enemy->GetData().kind == EnemyKind::Golem;
        int width = isLarge ? 120 : 60;
        int height = isLarge ? 8 : 5;
        int left = static_cast<int>(screen.x) - width / 2;
        int top = static_cast<int>(screen.y);

        float alpha = timer / 0.5f;
        if (alpha > 1.0f) alpha = 1.0f;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 200.0f));
        DrawBox(left - 1, top - 1, left + width + 1, top + height + 1, 0x000000, TRUE);
        DrawBox(left, top, left + static_cast<int>(width * enemy->GetHpRatio()), top + height, 0xE04848, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
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

        GameFont::DrawCentered(static_cast<int>(screen.x), static_cast<int>(screen.y) - 26,
            character->GetStateName(), color, GameFont::Size::Small);
    }
}
