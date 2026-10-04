#include "LockOnMarker.h"
#include "PlayerController.h"
#include "Enemy.h"
#include "GameFont.h"
#include "Time.h"
#include "DxLib.h"
#include <cmath>

namespace {
    // ほかの表示より手前に描く
    constexpr int SORTING_ORDER = 1;

    constexpr unsigned int MARKER_COLOR = 0xFF7A30;
    constexpr unsigned int OUTLINE_COLOR = 0x301008;

    // 印の大きさ 画面のドット
    constexpr float RETICLE_RADIUS = 34.0f;
    constexpr float ARROW_LENGTH = 16.0f;
    constexpr float ARROW_HALF_WIDTH = 9.0f;

    // 付けた瞬間にどれだけ外から縮んでくるか
    constexpr float APPEAR_EXTRA_RADIUS = 40.0f;
    constexpr float APPEAR_TIME = 0.18f;

    // 回る速さ 度/秒
    constexpr float SPIN_SPEED = 90.0f;

    // 相手の名前を出す高さ 画面の上の真ん中
    constexpr int NAME_TOP = 44;
}

void LockOnMarker::Setup(const PlayerController* controller) {
    _controller = controller;
    sortingOrder = SORTING_ORDER;
}

void LockOnMarker::Update(float deltaTime) {
    // ヒットストップ中も印は動かしたいので実時間で進める
    float unscaled = Time::UnscaledDeltaTime();

    _spinDegree += SPIN_SPEED * unscaled;
    if (_spinDegree >= 360.0f) _spinDegree -= 360.0f;

    const Character* target = _controller ? _controller->GetLockOnTarget() : nullptr;
    if (target != _shownTarget) {
        _shownTarget = target;
        _appear = 1.0f;
    }

    _appear -= unscaled / APPEAR_TIME;
    if (_appear < 0.0f) _appear = 0.0f;
}

void LockOnMarker::Render() {
    if (!_controller) return;

    // PlayerController がこのフレームに確かめた相手なので、まだ消えてはいない
    // 同じフレームのうちに倒されていることはあるので、そのときは出さない
    const Character* target = _controller->GetLockOnTarget();
    if (!target || target->IsDead()) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    // カメラの後ろにいるときは印を出さない 上の名前だけ出す
    VECTOR screen = ConvWorldPosToScreenPos(target->GetCenter());
    if (screen.z > 0.0f && screen.z < 1.0f) {
        DrawReticle(screen.x, screen.y);
    }

    DrawTargetName(*target, screenWidth);
}

void LockOnMarker::DrawReticle(float x, float y) const {
    // 付けた瞬間は大きく、すぐに縮んで収まる
    float radius = RETICLE_RADIUS + APPEAR_EXTRA_RADIUS * _appear * _appear;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
    for (int i = 0; i < 4; ++i) {
        float angle = (_spinDegree + 90.0f * i) * DX_PI_F / 180.0f;
        float dx = cosf(angle);
        float dy = sinf(angle);

        // 先を内側へ向けて、相手を指す三角にする
        float tipX = x + dx * radius;
        float tipY = y + dy * radius;
        float baseX = x + dx * (radius + ARROW_LENGTH);
        float baseY = y + dy * (radius + ARROW_LENGTH);
        float sideX = -dy * ARROW_HALF_WIDTH;
        float sideY = dx * ARROW_HALF_WIDTH;

        DrawTriangleAA(tipX, tipY, baseX + sideX, baseY + sideY, baseX - sideX, baseY - sideY, MARKER_COLOR, TRUE);

        // 縁取り 明るい地面や火花の上でも見えるように
        DrawTriangleAA(tipX, tipY, baseX + sideX, baseY + sideY, baseX - sideX, baseY - sideY, OUTLINE_COLOR, FALSE, 2.0f);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void LockOnMarker::DrawTargetName(const Character& target, int screenWidth) const {
    // 敵なら名前を出す Golem のような強い敵を狙っていると分かるように
    // 体力はなく、吹っ飛ばされ値も数字では出さないので、名前だけにしてある
    const auto* enemy = dynamic_cast<const Enemy*>(&target);
    if (!enemy) return;

    GameFont::DrawCentered(screenWidth / 2, NAME_TOP, enemy->GetData().displayName, 0xFFFFFF, GameFont::Size::Small);
}
