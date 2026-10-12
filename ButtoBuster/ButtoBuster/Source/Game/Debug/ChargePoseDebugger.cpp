#include "ChargePoseDebugger.h"
#include "Player.h"
#include "Animator.h"
#include "AnimationClip.h"
#include "InputSystem.h"
#include "GameFont.h"
#include "Time.h"
#include "DxLib.h"
#include <cstdio>

namespace {
    // 表示の見た目 コンボの表示と重ならない、画面の左の中ほどに出す
    struct PanelLook {
        int left;
        int padding;
        int lineGap;                // 文字の高さに足す行の間
        int backAlpha;              // 後ろを暗くする濃さ 0〜255
        unsigned int backColor;
        unsigned int textColor;
        unsigned int valueColor;    // 今の値 (アニメとフレーム) の行
    };
    constexpr PanelLook PANEL = { 40, 10, 6, 170, 0x000000, 0xE8E8E8, 0xFFD060 };

    // ほかの表示より手前に出す
    constexpr int SORTING_ORDER = 100;
}

void ChargePoseDebugger::Setup(Player* player) {
    _player = player;
    sortingOrder = SORTING_ORDER;
}

void ChargePoseDebugger::Update(float deltaTime) {
    if (!_player) return;

    const auto& input = InputSystem::Instance();

    if (input.KeyPressed(KEY_INPUT_P)) {
        bool isBack = input.KeyHeld(KEY_INPUT_LSHIFT) || input.KeyHeld(KEY_INPUT_RSHIFT);
        ChangeAnimation(isBack ? -1 : 1);
    }

    UpdateStep();
}

void ChargePoseDebugger::Render() {
    if (!_isActive || !_player) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    // 何番目のアニメかも出す 全部見終わったかが分かるように 1 から数え、無ければ 0
    int count = 0;
    if (Animator* animator = _player->GetAnimator()) count = static_cast<int>(animator->GetClipNames().size());

    char animationLine[128];
    snprintf(animationLine, sizeof(animationLine), "アニメ    %s  (%d / %d)",
        _player->data.chargePoseAnimation.c_str(), FindClipIndex() + 1, count);

    char timeLine[128];
    snprintf(timeLine, sizeof(timeLine), "フレーム  %.1f / %.1f", _player->data.chargePoseTime, GetTotalTime());

    const char* lines[] = {
        "溜めの姿勢 (制作用)",
        animationLine,
        timeLine,
        "P 次のアニメ   Shift + P 前のアニメ",
        ", 戻す   . 進める   押し続けると続けて動く",
        "溜めている間は、その場で姿勢が変わる",
        "決まったら PlayerData.h の chargePoseAnimation と chargePoseTime に書く",
    };
    constexpr int LINE_COUNT = sizeof(lines) / sizeof(lines[0]);

    int lineHeight = GameFont::GetHeight(GameFont::Size::Small) + PANEL.lineGap;
    int top = screenHeight / 2 - lineHeight * LINE_COUNT / 2;

    // 背景の幅は一番長い行に合わせる
    int width = 0;
    for (const char* line : lines) {
        int lineWidth = GameFont::GetWidth(line, GameFont::Size::Small);
        if (lineWidth > width) width = lineWidth;
    }

    // 3D の上でも読めるよう、後ろを暗くする
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, PANEL.backAlpha);
    DrawBox(PANEL.left - PANEL.padding, top - PANEL.padding,
        PANEL.left + width + PANEL.padding, top + lineHeight * LINE_COUNT + PANEL.padding, PANEL.backColor, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    for (int i = 0; i < LINE_COUNT; ++i) {
        bool isValue = lines[i] == animationLine || lines[i] == timeLine;
        GameFont::Draw(PANEL.left, top + lineHeight * i, lines[i], isValue ? PANEL.valueColor : PANEL.textColor, GameFont::Size::Small);
    }
}

void ChargePoseDebugger::ChangeAnimation(int direction) {
    Animator* animator = _player->GetAnimator();
    if (!animator) return;

    const auto& names = animator->GetClipNames();
    int count = static_cast<int>(names.size());
    if (count == 0) return;

    // 今のアニメが一覧に無ければ、先頭から数える
    int index = FindClipIndex();
    if (index < 0) index = 0;
    index = (index + direction + count) % count;
    _player->data.chargePoseAnimation = names[index];

    // 長さの違うアニメに替えたときも、止める時間がアニメの終わりを越えないように
    _player->data.chargePoseTime = ClampToClip(_player->data.chargePoseTime);

    _isActive = true;
}

void ChargePoseDebugger::StepTime(float amount) {
    float time = _player->data.chargePoseTime + amount;
    if (time < 0.0f) time = 0.0f;

    _player->data.chargePoseTime = ClampToClip(time);
    _isActive = true;
}

void ChargePoseDebugger::UpdateStep() {
    const auto& input = InputSystem::Instance();

    float direction = 0.0f;
    if (input.KeyHeld(KEY_INPUT_COMMA)) direction -= 1.0f;
    if (input.KeyHeld(KEY_INPUT_PERIOD)) direction += 1.0f;

    if (direction == 0.0f) {
        _repeatTimer = 0.0f;
        return;
    }

    // 押した瞬間に 1 回動かし、押し続けたら少し待ってから続けて動かす
    if (input.KeyPressed(KEY_INPUT_COMMA) || input.KeyPressed(KEY_INPUT_PERIOD)) {
        StepTime(direction * FRAME_STEP);
        _repeatTimer = REPEAT_DELAY;
        return;
    }

    // スローやヒットストップの間も同じ速さで動かせるよう、実時間で数える
    _repeatTimer -= Time::UnscaledDeltaTime();
    if (_repeatTimer > 0.0f) return;

    StepTime(direction * FRAME_STEP);
    _repeatTimer = REPEAT_INTERVAL;
}

float ChargePoseDebugger::GetTotalTime() const {
    Animator* animator = _player->GetAnimator();
    if (!animator) return 0.0f;

    const AnimationClip* clip = animator->GetClip(_player->data.chargePoseAnimation);
    return clip ? clip->TotalTime : 0.0f;
}

int ChargePoseDebugger::FindClipIndex() const {
    Animator* animator = _player->GetAnimator();
    if (!animator) return -1;

    const auto& names = animator->GetClipNames();
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        if (names[i] == _player->data.chargePoseAnimation) return i;
    }
    return -1;
}

float ChargePoseDebugger::ClampToClip(float time) const {
    float totalTime = GetTotalTime();
    return (totalTime > 0.0f && time > totalTime) ? totalTime : time;
}
