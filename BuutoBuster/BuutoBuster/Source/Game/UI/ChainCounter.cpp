#include "ChainCounter.h"
#include "GameFont.h"
#include "Time.h"
#include "DxLib.h"
#include <cstdio>

namespace {
    // ロックオンした敵の体力の下 画面の上の真ん中に出す
    constexpr int TOP = 84;

    // フェーズの表示より奥、結果の画面より奥に描く
    constexpr int SORTING_ORDER = 8;

    // 連鎖が伸びるほど赤く、大きくする
    unsigned int GetColor(int count) {
        if (count >= ChainMilestone::THIRD) return 0xFFD700;
        if (count >= ChainMilestone::SECOND) return 0xFF6A30;
        if (count >= ChainMilestone::FIRST) return 0xFFB040;
        return 0xFFE6A0;
    }
}

void ChainCounter::Setup() {
    sortingOrder = SORTING_ORDER;
}

void ChainCounter::OnNotify(const ChainEvent& event) {
    if (event.type == ChainEvent::Type::End) {
        // 今出している連鎖の終わりだけを受け取る
        // 巻き込まずに終わった別の振りの知らせで、前の結果の表示が延びないように
        if (event.count != _count) return;

        _hasEnded = true;
        _timer = 0.0f;
        return;
    }

    _count = event.count;
    _hasEnded = false;
    _timer = 0.0f;
    _bounceTimer = BOUNCE_TIME;
}

void ChainCounter::Update(float deltaTime) {
    if (_count == 0) return;

    // ヒットストップやスローの間も同じ速さで消えるよう、実時間で数える
    float unscaled = Time::UnscaledDeltaTime();
    _timer += unscaled;
    if (_bounceTimer > 0.0f) _bounceTimer -= unscaled;

    if (GetAlpha() <= 0.0f) {
        _count = 0;
        _hasEnded = false;
    }
}

void ChainCounter::Render() {
    if (_count < MIN_SHOWN_COUNT) return;

    float alpha = GetAlpha();
    if (alpha <= 0.0f) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    float bounceRate = (_bounceTimer > 0.0f) ? _bounceTimer / BOUNCE_TIME : 0.0f;
    int top = TOP - static_cast<int>(BOUNCE_HEIGHT * bounceRate);

    // 止まったら ! を付けて、その振りの結果として見せる
    char text[32];
    snprintf(text, sizeof(text), _hasEnded ? "%d CHAIN!" : "%d CHAIN", _count);

    GameFont::Size size = (_count >= ChainMilestone::SECOND) ? GameFont::Size::Huge : GameFont::Size::Large;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
    GameFont::DrawCentered(screenWidth / 2, top, text, GetColor(_count), size);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

float ChainCounter::GetAlpha() const {
    // 止まってから少し見せてから消す 止まった知らせが来ないときは、静かになってから消す
    float fadeStart = _hasEnded ? HOLD_TIME : QUIET_TIME;
    if (_timer <= fadeStart) return 1.0f;

    float rate = 1.0f - (_timer - fadeStart) / FADE_TIME;
    return (rate > 0.0f) ? rate : 0.0f;
}
