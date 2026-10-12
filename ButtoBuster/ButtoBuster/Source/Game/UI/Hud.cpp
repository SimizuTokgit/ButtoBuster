#include "Hud.h"
#include "GameFont.h"
#include "ControlsList.h"
#include "Player.h"
#include "PhaseDirector.h"
#include "Enemy.h"
#include "CharacterRegistry.h"
#include "SoundManager.h"
#include "JustDodgeEvent.h"
#include "Mathf.h"
#include "UiDraw.h"
#include "UiLayer.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

namespace {
    constexpr unsigned int WHITE = 0xFFFFFF;

    // 画像で描くゲージの置き方 枠を置く場所 (左上) と、画像に掛ける倍率
    struct GaugeLayout {
        int x;
        int y;
        float scale;

        // 枠の画像の中の位置 (px) を、画面の位置にする
        int ToScreenX(float imageX) const { return x + static_cast<int>(imageX * scale); }
        int ToScreenY(float imageY) const { return y + static_cast<int>(imageY * scale); }

        // 枠の画像を、倍率を掛けた大きさで描く
        void DrawFrame(int graph, int width, int height) const {
            DrawExtendGraph(x, y, ToScreenX(static_cast<float>(width)), ToScreenY(static_cast<float>(height)), graph, TRUE);
        }
    };

    // ----- 回避ゲージ (左上) -----
    // 続けて回避できる回数 (PlayerData の dodgeCount) の 2 つの枠に、1 回分ずつ中身が入る
    // 画像は Data/2D の DodgeGauge で始まるもの 枠の左の菱形 (回避の紋章) も枠の画像に入っている
    constexpr const char* DODGE_FRAME_IMAGE = "Data/2D/DodgeGaugeFrame.png";
    constexpr const char* DODGE_FILL_IMAGE = "Data/2D/DodgeGaugeFill.png";

    // 画像の枠の数 dodgeCount がこれと違うときは、下の四角のバーで出す
    constexpr int DODGE_SLOT_COUNT = 2;

    // 枠を置く場所 (左上) と大きさ 画像 (1190 x 215) に掛ける倍率
    constexpr GaugeLayout DODGE_GAUGE = { 20, 4, 0.32f };

    // 中身を入れる枠穴 枠の画像の中の位置 (px) 左の端は 1 つ目と 2 つ目 上の端は同じ
    constexpr int DODGE_SLOT_LEFTS[DODGE_SLOT_COUNT] = { 213, 628 };
    constexpr int DODGE_SLOT_TOP = 79;

    // 戻っている途中の中身の濃さ 0〜255 溜まりきると 255 になる
    constexpr int DODGE_CHARGING_ALPHA = 120;

    // ----- 回避の残りのバー (画像が使えないときの四角のバー) -----
    // 画像の回避ゲージのかわりに左上に出す 続けて回避できる回数 (PlayerData の dodgeCount) に分け、1 つが 1 回分
    constexpr int DODGE_BAR_X = 40;
    constexpr int DODGE_BAR_Y = 34;
    constexpr int DODGE_BAR_WIDTH = 380;
    constexpr int DODGE_BAR_HEIGHT = 22;

    // 分けたバーの間の隙間
    constexpr int DODGE_BAR_GAP = 8;

    // 使える分の色は、ジャスト回避と同じ水色 (JUST_DODGE_COLOR) 溜まりきるとこの色に変わる
    // 戻っている途中の色
    constexpr unsigned int DODGE_CHARGING_COLOR = 0x3A7088;

    // バーの後ろに敷く黒の濃さ 0〜255 と、バーより外へ広げる幅 3D の上でも読みやすくする
    constexpr int DODGE_BAR_SHADE_ALPHA = 170;
    constexpr int DODGE_BAR_SHADE_MARGIN = 4;

    // バーの下に出す名前と、バーとの間
    constexpr const char* DODGE_BAR_LABEL = "回避";
    constexpr int DODGE_BAR_LABEL_GAP = 6;

    // ----- バースターゲージ (必殺技のゲージ 左上の回避ゲージの下) -----
    // 当てた数 (コンボ) 連鎖 壁割りで溜まる 満タンになるとパッド Y / キーボード F で必殺技を撃てる
    // どれだけ溜まるかは PlayerData の specialGain で始まる値
    // 画像は Data/2D の BusterGauge で始まるものと BusterLogo 中身は枠穴の傾きに合わせて傾け直してある
    constexpr const char* SPECIAL_FRAME_IMAGE = "Data/2D/BusterGaugeFrame.png";
    constexpr const char* SPECIAL_FILL_IMAGE = "Data/2D/BusterGaugeFill.png";
    constexpr const char* SPECIAL_LOGO_IMAGE = "Data/2D/BusterLogo.png";

    // 枠を置く場所 (左上) と大きさ 画像 (1157 x 278) に掛ける倍率
    // 紋章が回避ゲージの紋章の真下に来るようにしてある
    constexpr GaugeLayout SPECIAL_GAUGE = { 14, 68, 0.33f };

    // 中身を入れる所 枠の画像の中の位置 (px) 中身の画像 (788 x 74) をこの四角に合わせて伸ばす
    constexpr int SPECIAL_FILL_LEFT = 267;
    constexpr int SPECIAL_FILL_TOP = 97;
    constexpr int SPECIAL_FILL_RIGHT = 1055;
    constexpr int SPECIAL_FILL_BOTTOM = 171;

    // 溜まっている途中の中身の濃さ 0〜255 満タンになると 255 で光る
    constexpr int SPECIAL_FILL_ALPHA = 200;

    // 満タンの間、中身に重ねる白い光の濃さ 0〜255 と、1 秒に光る回数
    constexpr int SPECIAL_READY_GLOW_ALPHA = 120;
    constexpr float SPECIAL_READY_BEATS_PER_SECOND = 1.5f;

    // 満タンで出る BUSTER!! の文字 イメージ図 (⑥) と同じく、紋章の右、枠の上に重ねる
    // 文字の真ん中の位置と幅は、枠の画像の中の px で書く
    constexpr int SPECIAL_LOGO_CENTER_X = 450;
    constexpr int SPECIAL_LOGO_CENTER_Y = 45;
    constexpr int SPECIAL_LOGO_WIDTH = 380;

    // 文字の出方 この倍率の大きさから、この秒数で元の大きさへ縮みながら現れる 出た瞬間は白く光らせる
    constexpr float SPECIAL_LOGO_POP_SCALE = 2.2f;
    constexpr float SPECIAL_LOGO_POP_TIME = 0.25f;
    constexpr float SPECIAL_LOGO_FLASH_TIME = 0.4f;

    // 出たあと、文字をゆっくり脈打たせる大きさの幅 (0.04 で ±4%)
    constexpr float SPECIAL_LOGO_PULSE = 0.04f;

    // 満タンのとき枠の下に出す、押すボタン 空なら出さない 枠の下の端との間
    constexpr const char* SPECIAL_READY_HINT = "F / Y";
    constexpr unsigned int SPECIAL_READY_HINT_COLOR = 0xA8E0FF;
    constexpr int SPECIAL_READY_HINT_GAP = 2;

    // ----- フェーズの表示 (右上) -----
    // 右の端は画面の右の端からの距離 行ごとの上の端の高さと色
    constexpr int PHASE_RIGHT_MARGIN = 40;
    constexpr int PHASE_Y = 20;
    constexpr int REMAINING_Y = 84;             // 残りの敵
    constexpr unsigned int REMAINING_COLOR = 0xE0E0E0;
    constexpr int GOAL_Y = 122;                 // 次の全回復か勝ちまで 勝ちのときは金
    constexpr unsigned int HEAL_GOAL_COLOR = 0x90F0B0;
    constexpr int KILL_Y = 148;                 // 撃破数
    constexpr unsigned int KILL_COLOR = 0xC0C0C0;

    // ----- コンボ (画面の右 色は金) -----
    constexpr int MIN_SHOWN_COMBO = 2;          // これより少ないときは出さない
    constexpr float COMBO_FADE_TIME = 0.5f;     // 途切れるまでの残りがこの秒数を切ると薄くしていく
    constexpr int COMBO_RIGHT_MARGIN = 130;     // 数の右の端 画面の右の端からの距離
    constexpr int COMBO_RAISE = 80;             // 数の上の端 画面の真ん中からこれだけ上
    constexpr int COMBO_HIT_X = 8;              // HIT の文字の左の端 数の右の端からの距離
    constexpr int COMBO_HIT_Y = 52;             // HIT の文字の上の端 数の上の端からの距離

    // ----- 操作の説明 (画面の左下) -----
    // 画像は Data/2D/Controls.png (1001 x 216) 読めないときは文字で出す
    constexpr const char* CONTROLS_IMAGE = "Data/2D/Controls.png";

    // 画面に出す幅 px 高さは画像の縦横の比で決まる 左の端と下の端からの隙間
    constexpr int CONTROLS_WIDTH = 720;
    constexpr int CONTROLS_LEFT_MARGIN = 8;
    constexpr int CONTROLS_BOTTOM_MARGIN = 2;

    // 濃さ 0〜255 少し透かして、後ろの戦いが見えるようにする
    constexpr int CONTROLS_ALPHA = 230;

    // 画像が読めないときの文字の説明で、ボタンの列を出す所 操作の名前の左の端からの距離
    constexpr int CONTROLS_KEYS_X = 150;

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
        { 0.5f,   50, 20, 110, 1.2f },   // 半分を越えた 赤みが見え始める
        { 0.75f,  90, 40, 180, 2.0f },   // 4 分の 3 を越えた はっきり赤く点滅
        { 1.0f,  140, 70, 240, 3.0f },   // 許容値に届いた 周りが真っ赤に、速く強く点滅
    };
    constexpr int DANGER_LOOK_COUNT = sizeof(DANGER_LOOKS) / sizeof(DANGER_LOOKS[0]);

    constexpr unsigned int DANGER_COLOR = 0xE01010;

    // 点滅の鋭さ 1 でなめらかに明暗を繰り返し、大きいほど暗い時間が長くなって一瞬強く光る (点滅らしくなる)
    constexpr float DANGER_BLINK_SHARPNESS = 2.5f;

    // 縁をこの数の帯に分け、内側ほど薄くしてぼかす
    constexpr int DANGER_BANDS = 12;

    // 許容値に届いている間、脈ごとに鳴らす音 鳴らすかどうかの切り替えを兼ねる
    // 今は合う音が無いので空にして鳴らさない 心音の音を Data/Sound/SE に入れたら名前を書くだけで鳴る
    constexpr const char* DANGER_BEAT_SOUND = "";
    constexpr float DANGER_BEAT_VOLUME = 0.8f;

    // ----- 制作用の状態の表示 (キャラの頭上) -----
    constexpr float STATE_HEAD_LIFT = 40.0f;    // 体のてっぺんからこれだけ上
    constexpr int STATE_LABEL_RAISE = 26;       // 文字の上の端 その位置からこれだけ上
    constexpr unsigned int STATE_COLOR = 0x80FFFF;
    constexpr unsigned int STATE_TOKEN_COLOR = 0xFF6060;   // 攻撃の番を持っている敵

    // 頭の上の画面の位置 カメラの後ろにあるなら false
    bool GetHeadScreenPosition(const Character& character, VECTOR& outScreen) {
        VECTOR head = VAdd(character.GetTopPosition(), VGet(0.0f, STATE_HEAD_LIFT, 0.0f));
        outScreen = ConvWorldPosToScreenPos(head);
        return outScreen.z > 0.0f && outScreen.z < 1.0f;
    }
}

Hud::~Hud() {
    if (_specialFrameGraph != -1) DeleteGraph(_specialFrameGraph);
    if (_specialFillGraph != -1) DeleteGraph(_specialFillGraph);
    if (_specialLogoGraph != -1) DeleteGraph(_specialLogoGraph);
    if (_dodgeFrameGraph != -1) DeleteGraph(_dodgeFrameGraph);
    if (_dodgeFillGraph != -1) DeleteGraph(_dodgeFillGraph);
    if (_controlsGraph != -1) DeleteGraph(_controlsGraph);
}

void Hud::Setup(Player* player, PhaseDirector* director) {
    _player = player;
    _director = director;
    sortingOrder = UiLayer::HUD;

    _specialFrameGraph = LoadGraph(SPECIAL_FRAME_IMAGE);
    _specialFillGraph = LoadGraph(SPECIAL_FILL_IMAGE);
    _specialLogoGraph = LoadGraph(SPECIAL_LOGO_IMAGE);
    _dodgeFrameGraph = LoadGraph(DODGE_FRAME_IMAGE);
    _dodgeFillGraph = LoadGraph(DODGE_FILL_IMAGE);
    _controlsGraph = LoadGraph(CONTROLS_IMAGE);
}

void Hud::Render() {
    if (!_player || !_director) return;

    int screenWidth = 0;
    int screenHeight = 0;
    GetDrawScreenSize(&screenWidth, &screenHeight);

    if (isStateVisible) DrawStates();

    DrawDanger(screenWidth, screenHeight);
    DrawDodgeStock();
    DrawSpecialGauge();

    // チュートリアルの間はフェーズの表示を出さない
    if (_director->GetStep() != PhaseDirector::Step::Practice) DrawPhaseInfo(screenWidth);

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

    // 0〜1 で脈打つ 鋭さを掛けて、一瞬強く光る点滅にする
    float seconds = GetNowCount() / 1000.0f;
    float pulse = (sinf(seconds * look.beatsPerSecond * DX_TWO_PI_F) + 1.0f) * 0.5f;
    pulse = powf(pulse, DANGER_BLINK_SHARPNESS);
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

    float stock = _player->GetDodgeStock();
    bool canUseImage = _dodgeFrameGraph != -1 && _dodgeFillGraph != -1 && count == DODGE_SLOT_COUNT;
    if (canUseImage) {
        int frameWidth = 0;
        int frameHeight = 0;
        GetGraphSize(_dodgeFrameGraph, &frameWidth, &frameHeight);
        int fillWidth = 0;
        int fillHeight = 0;
        GetGraphSize(_dodgeFillGraph, &fillWidth, &fillHeight);

        DODGE_GAUGE.DrawFrame(_dodgeFrameGraph, frameWidth, frameHeight);

        // 左から順に溜まる 戻っている途中の 1 つは、溜まった分だけ左から伸び、薄く出す
        for (int i = 0; i < DODGE_SLOT_COUNT; ++i) {
            float fill = Mathf::Clamp01(stock - static_cast<float>(i));
            if (fill <= 0.0f) continue;

            int sourceWidth = static_cast<int>(fillWidth * fill);
            if (sourceWidth <= 0) continue;

            float left = static_cast<float>(DODGE_SLOT_LEFTS[i]);
            int x1 = DODGE_GAUGE.ToScreenX(left);
            int y1 = DODGE_GAUGE.ToScreenY(static_cast<float>(DODGE_SLOT_TOP));
            int x2 = DODGE_GAUGE.ToScreenX(left + sourceWidth);
            int y2 = DODGE_GAUGE.ToScreenY(static_cast<float>(DODGE_SLOT_TOP + fillHeight));

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, (fill >= 1.0f) ? 255 : DODGE_CHARGING_ALPHA);
            DrawRectExtendGraph(x1, y1, x2, y2, 0, 0, sourceWidth, fillHeight, _dodgeFillGraph, TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        return;
    }

    int segmentWidth = (DODGE_BAR_WIDTH - DODGE_BAR_GAP * (count - 1)) / count;
    int bottom = DODGE_BAR_Y + DODGE_BAR_HEIGHT;

    UiDraw::FillBox(DODGE_BAR_X - DODGE_BAR_SHADE_MARGIN, DODGE_BAR_Y - DODGE_BAR_SHADE_MARGIN,
        DODGE_BAR_X + DODGE_BAR_WIDTH + DODGE_BAR_SHADE_MARGIN, bottom + DODGE_BAR_SHADE_MARGIN, 0x000000, DODGE_BAR_SHADE_ALPHA);

    // 左から順に溜まる 戻っている途中の 1 つは、溜まった分だけ伸びる
    for (int i = 0; i < count; ++i) {
        int left = DODGE_BAR_X + (segmentWidth + DODGE_BAR_GAP) * i;

        float fill = Mathf::Clamp01(stock - static_cast<float>(i));
        unsigned int color = (fill >= 1.0f) ? JUST_DODGE_COLOR : DODGE_CHARGING_COLOR;
        UiDraw::DrawBar(left, DODGE_BAR_Y, segmentWidth, DODGE_BAR_HEIGHT, fill, color, WHITE);
    }

    GameFont::Draw(DODGE_BAR_X, bottom + DODGE_BAR_LABEL_GAP, DODGE_BAR_LABEL, WHITE, GameFont::Size::Small);
}

void Hud::DrawSpecialGauge() {
    if (_specialFrameGraph == -1) return;

    float ratio = Mathf::Clamp01(_player->GetSpecialRatio());
    bool isReady = _player->IsSpecialReady();

    // 満タンになった瞬間を覚えておき、そこから文字を出す
    int now = GetNowCount();
    if (isReady && !_wasSpecialReady) _specialReadyTime = now;
    _wasSpecialReady = isReady;

    int frameWidth = 0;
    int frameHeight = 0;
    GetGraphSize(_specialFrameGraph, &frameWidth, &frameHeight);

    // 中身 左から溜まった分だけ、画像も同じ割合だけ切り取って伸ばす
    int fillLeft = SPECIAL_GAUGE.ToScreenX(SPECIAL_FILL_LEFT);
    int fillTop = SPECIAL_GAUGE.ToScreenY(SPECIAL_FILL_TOP);
    int fillRight = SPECIAL_GAUGE.ToScreenX(SPECIAL_FILL_RIGHT);
    int fillBottom = SPECIAL_GAUGE.ToScreenY(SPECIAL_FILL_BOTTOM);
    if (_specialFillGraph != -1 && ratio > 0.0f) {
        int fillWidth = 0;
        int fillHeight = 0;
        GetGraphSize(_specialFillGraph, &fillWidth, &fillHeight);

        int right = fillLeft + static_cast<int>((fillRight - fillLeft) * ratio);
        int sourceWidth = static_cast<int>(fillWidth * ratio);
        if (sourceWidth > 0) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, isReady ? 255 : SPECIAL_FILL_ALPHA);
            DrawRectExtendGraph(fillLeft, fillTop, right, fillBottom, 0, 0, sourceWidth, fillHeight, _specialFillGraph, TRUE);

            // 満タンの間は同じ画像を光らせて重ね、脈打たせる
            if (isReady) {
                float seconds = now / 1000.0f;
                float pulse = (sinf(seconds * SPECIAL_READY_BEATS_PER_SECOND * DX_TWO_PI_F) + 1.0f) * 0.5f;
                SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(SPECIAL_READY_GLOW_ALPHA * pulse));
                DrawRectExtendGraph(fillLeft, fillTop, right, fillBottom, 0, 0, sourceWidth, fillHeight, _specialFillGraph, TRUE);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // 枠は中身の上に重ねる
    SPECIAL_GAUGE.DrawFrame(_specialFrameGraph, frameWidth, frameHeight);

    if (!isReady) return;

    if (SPECIAL_READY_HINT[0] != '\0') {
        GameFont::DrawCentered((fillLeft + fillRight) / 2, SPECIAL_GAUGE.ToScreenY(static_cast<float>(frameHeight)) + SPECIAL_READY_HINT_GAP,
            SPECIAL_READY_HINT, SPECIAL_READY_HINT_COLOR, GameFont::Size::Small);
    }

    // BUSTER!! の文字 大きく出て縮みながら現れ、白く光ってから、ゆっくり脈打つ
    if (_specialLogoGraph == -1) return;

    float elapsed = (now - _specialReadyTime) / 1000.0f;
    int logoWidth = 0;
    int logoHeight = 0;
    GetGraphSize(_specialLogoGraph, &logoWidth, &logoHeight);
    if (logoWidth <= 0) return;

    float scale = SPECIAL_LOGO_WIDTH * SPECIAL_GAUGE.scale / logoWidth;
    int alpha = 255;
    if (elapsed < SPECIAL_LOGO_POP_TIME) {
        float t = elapsed / SPECIAL_LOGO_POP_TIME;
        float eased = Mathf::EaseOutQuad(t);
        scale *= SPECIAL_LOGO_POP_SCALE + (1.0f - SPECIAL_LOGO_POP_SCALE) * eased;
        alpha = static_cast<int>(255.0f * t);
    }
    else {
        float pulse = sinf(elapsed * SPECIAL_READY_BEATS_PER_SECOND * DX_TWO_PI_F);
        scale *= 1.0f + SPECIAL_LOGO_PULSE * pulse;
    }

    int centerX = SPECIAL_GAUGE.ToScreenX(SPECIAL_LOGO_CENTER_X);
    int centerY = SPECIAL_GAUGE.ToScreenY(SPECIAL_LOGO_CENTER_Y);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawRotaGraph(centerX, centerY, scale, 0.0, _specialLogoGraph, TRUE);

    // 出た瞬間は、同じ文字を光らせて重ねる
    if (elapsed < SPECIAL_LOGO_FLASH_TIME) {
        float flash = 1.0f - elapsed / SPECIAL_LOGO_FLASH_TIME;
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255.0f * flash));
        DrawRotaGraph(centerX, centerY, scale, 0.0, _specialLogoGraph, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Hud::DrawPhaseInfo(int screenWidth) {
    int right = screenWidth - PHASE_RIGHT_MARGIN;
    char text[64];

    snprintf(text, sizeof(text), "PHASE %d", _director->GetPhase());
    GameFont::DrawRight(right, PHASE_Y, text, WHITE, GameFont::Size::Large);

    snprintf(text, sizeof(text), "残り %d", _director->GetRemainingEnemyCount());
    GameFont::DrawRight(right, REMAINING_Y, text, REMAINING_COLOR, GameFont::Size::Medium);

    // 全回復より先に勝ちが来るなら、勝ちまでを出す 最後のフェーズを越えたときは全回復しないので
    int untilHeal = _director->GetPhasesUntilHeal();
    int untilVictory = _director->GetPhasesUntilVictory();
    unsigned int goalColor = HEAL_GOAL_COLOR;
    if (untilVictory >= 0 && untilVictory <= untilHeal) {
        goalColor = UiDraw::GOLD;
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
    GameFont::DrawRight(right, GOAL_Y, text, goalColor, GameFont::Size::Small);

    snprintf(text, sizeof(text), "撃破 %d", _director->GetKillCount());
    GameFont::DrawRight(right, KILL_Y, text, KILL_COLOR, GameFont::Size::Small);
}

void Hud::DrawCombo(int screenWidth, int screenHeight) {
    int combo = _player->GetCombo();
    if (combo < MIN_SHOWN_COMBO) return;

    // 途切れる直前に薄くして、あと少しで切れることを知らせる
    float alpha = Mathf::Clamp01(_player->GetComboTimer() / COMBO_FADE_TIME);

    char text[16];
    snprintf(text, sizeof(text), "%d", combo);

    int right = screenWidth - COMBO_RIGHT_MARGIN;
    int top = screenHeight / 2 - COMBO_RAISE;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 255.0f));
    GameFont::DrawRight(right, top, text, UiDraw::GOLD, GameFont::Size::Huge);
    GameFont::Draw(right + COMBO_HIT_X, top + COMBO_HIT_Y, "HIT", UiDraw::GOLD, GameFont::Size::Medium);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Hud::DrawControls(int screenWidth, int screenHeight) {
    if (_controlsGraph != -1) {
        int imageWidth = 0;
        int imageHeight = 0;
        GetGraphSize(_controlsGraph, &imageWidth, &imageHeight);
        if (imageWidth > 0) {
            int height = CONTROLS_WIDTH * imageHeight / imageWidth;
            int left = CONTROLS_LEFT_MARGIN;
            int top = screenHeight - CONTROLS_BOTTOM_MARGIN - height;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, CONTROLS_ALPHA);
            DrawExtendGraph(left, top, left + CONTROLS_WIDTH, top + height, _controlsGraph, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            return;
        }
    }

    // 画像が読めないときは、タイトルと同じ操作の一覧 (ControlsList.h) を、操作とボタンの 2 列で左下に出す
    int lineHeight = GameFont::GetHeight(GameFont::Size::Tiny);
    int top = screenHeight - CONTROLS_BOTTOM_MARGIN - lineHeight * ControlsList::ROW_COUNT;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, CONTROLS_ALPHA);
    for (int i = 0; i < ControlsList::ROW_COUNT; ++i) {
        const ControlsList::Row& row = ControlsList::ROWS[i];
        int y = top + lineHeight * i;
        GameFont::Draw(CONTROLS_LEFT_MARGIN, y, row.name, WHITE, GameFont::Size::Tiny);
        GameFont::Draw(CONTROLS_LEFT_MARGIN + CONTROLS_KEYS_X, y, row.keys, WHITE, GameFont::Size::Tiny);
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
        unsigned int color = STATE_COLOR;
        char state[64];
        snprintf(state, sizeof(state), "%s", character->GetStateName());
        if (const auto* enemy = dynamic_cast<const Enemy*>(character)) {
            if (_director->GetTokens().Has(enemy)) color = STATE_TOKEN_COLOR;
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
        GameFont::DrawCentered(static_cast<int>(screen.x), static_cast<int>(screen.y) - STATE_LABEL_RAISE,
            label, color, GameFont::Size::Small);
    }
}
