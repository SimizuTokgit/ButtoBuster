#pragma once

// 画面の表示を描く順 UIImage の sortingOrder に入れる 小さいほど先に (奥に) 描く
// 描く順はここだけで決める ほかの表示との前後を変えたいときは、ここの数を見比べて変える
namespace UiLayer {
    // スローの間に画面を暗くする幕 (SlowShade) いちばん奥
    constexpr int SLOW_SHADE = -10;

    // 画面の光 (ScreenFlash)
    constexpr int SCREEN_FLASH = -5;

    // 戦いの表示 (Hud)
    constexpr int HUD = 0;

    // 戦いの最中に出る知らせの文字 連鎖の数 (ChainCounter) と JUST DODGE! (JustDodgeBanner)
    constexpr int POPUP = 8;

    // フェーズの区切り (PhaseBanner)
    constexpr int PHASE_BANNER = 10;

    // チュートリアルの指示 (TutorialScreen)
    constexpr int TUTORIAL = 12;

    // 結果の画面 (ResultScreen) いちばん手前
    constexpr int RESULT = 20;
}
