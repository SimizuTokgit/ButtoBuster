#include "JustDodgeBanner.h"
#include "GameFont.h"
#include "Time.h"
#include "UiLayer.h"

namespace {
    // 色はかわしたと分かる水色 (JUST_DODGE_COLOR) 反撃の弧と同じ色にする
    const char* const TEXT = "JUST DODGE!";
}

void JustDodgeBanner::Setup() {
    sortingOrder = UiLayer::POPUP;
}

void JustDodgeBanner::OnNotify(const JustDodgeEvent& event) {
    _worldPosition = event.headPosition;
    _timer = SHOW_TIME;
}

void JustDodgeBanner::Update(float deltaTime) {
    // 全体がゆっくりになっている間に出すので、実時間で数える
    if (_timer > 0.0f) _timer -= Time::UnscaledDeltaTime();
}

void JustDodgeBanner::Render() {
    if (_timer <= 0.0f) return;

    // カメラの後ろにあるときは出さない
    VECTOR screen = ConvWorldPosToScreenPos(_worldPosition);
    if (screen.z <= 0.0f || screen.z >= 1.0f) return;

    float elapsed = SHOW_TIME - _timer;
    float alpha = (_timer < FADE_TIME) ? _timer / FADE_TIME : 1.0f;

    // 出た瞬間は大きな文字で目を引き、すぐ普段の大きさに戻す
    GameFont::Size size = (elapsed < POP_TIME) ? GameFont::Size::Huge : GameFont::Size::Large;

    // 頭の上から少しずつ浮き上がる 文字の下の端を頭の上に合わせる
    float rise = RISE_HEIGHT * (elapsed / SHOW_TIME);
    int top = static_cast<int>(screen.y - rise) - GameFont::GetHeight(size);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
    GameFont::DrawCentered(static_cast<int>(screen.x), top, TEXT, JUST_DODGE_COLOR, size);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
