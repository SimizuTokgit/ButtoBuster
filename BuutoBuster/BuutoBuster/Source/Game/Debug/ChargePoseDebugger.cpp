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
    // 体力やコンボの表示と重ならない、画面の左の中ほどに出す
    constexpr int LEFT = 40;
    constexpr int PADDING = 10;

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

    // 何番目のアニメかも出す 全部見終わったかが分かるように
    int index = 0;
    int count = 0;
    if (Animator* animator = _player->GetAnimator()) {
        const auto& names = animator->GetClipNames();
        count = static_cast<int>(names.size());
        for (int i = 0; i < count; ++i) {
            if (names[i] == _player->params.chargePoseAnimation) index = i + 1;
        }
    }

    char animationLine[128];
    snprintf(animationLine, sizeof(animationLine), "アニメ    %s  (%d / %d)",
        _player->params.chargePoseAnimation.c_str(), index, count);

    char timeLine[128];
    snprintf(timeLine, sizeof(timeLine), "フレーム  %.1f / %.1f", _player->params.chargePoseTime, GetTotalTime());

    const char* lines[] = {
        "溜めの姿勢 (制作用)",
        animationLine,
        timeLine,
        "P 次のアニメ   Shift + P 前のアニメ",
        ", 戻す   . 進める   押し続けると続けて動く",
        "溜めている間は、その場で姿勢が変わる",
        "決まったら PlayerParams.h の chargePoseAnimation と chargePoseTime に書く",
    };
    constexpr int LINE_COUNT = sizeof(lines) / sizeof(lines[0]);

    int lineHeight = GameFont::GetHeight(GameFont::Size::Small) + 6;
    int top = screenHeight / 2 - lineHeight * LINE_COUNT / 2;

    // 背景の幅は一番長い行に合わせる
    int width = 0;
    for (const char* line : lines) {
        int lineWidth = GameFont::GetWidth(line, GameFont::Size::Small);
        if (lineWidth > width) width = lineWidth;
    }

    // 3D の上でも読めるよう、後ろを暗くする
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
    DrawBox(LEFT - PADDING, top - PADDING, LEFT + width + PADDING, top + lineHeight * LINE_COUNT + PADDING, 0x000000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    for (int i = 0; i < LINE_COUNT; ++i) {
        unsigned int color = (i == 1 || i == 2) ? 0xFFD060 : 0xE8E8E8;
        GameFont::Draw(LEFT, top + lineHeight * i, lines[i], color, GameFont::Size::Small);
    }
}

void ChargePoseDebugger::ChangeAnimation(int direction) {
    Animator* animator = _player->GetAnimator();
    if (!animator) return;

    const auto& names = animator->GetClipNames();
    int count = static_cast<int>(names.size());
    if (count == 0) return;

    int index = 0;
    for (int i = 0; i < count; ++i) {
        if (names[i] == _player->params.chargePoseAnimation) {
            index = i;
            break;
        }
    }
    index = (index + direction + count) % count;
    _player->params.chargePoseAnimation = names[index];

    // 長さの違うアニメに替えたとき、止める時間がアニメの終わりを越えないようにする
    float totalTime = GetTotalTime();
    if (totalTime > 0.0f && _player->params.chargePoseTime > totalTime) _player->params.chargePoseTime = totalTime;

    _isActive = true;
}

void ChargePoseDebugger::StepTime(float amount) {
    float time = _player->params.chargePoseTime + amount;
    if (time < 0.0f) time = 0.0f;

    float totalTime = GetTotalTime();
    if (totalTime > 0.0f && time > totalTime) time = totalTime;

    _player->params.chargePoseTime = time;
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

    const AnimationClip* clip = animator->GetClip(_player->params.chargePoseAnimation);
    return clip ? clip->TotalTime : 0.0f;
}
