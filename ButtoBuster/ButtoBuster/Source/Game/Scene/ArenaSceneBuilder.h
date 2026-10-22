#pragma once

class GameObject;
class CameraFollow;
class Player;
class PlayerController;

// 本番 (GameScene) とチュートリアル (TutorialScene) で同じ、闘技場の組み立て
// カメラ 光 聞き手 ステージ 演出 壁割りの演出と音 針の置き場 プレイヤー をこの順で作る
// 進め役と画面の表示は場面ごとに違うので、それぞれのシーンが作る
namespace ArenaSceneBuilder {
    // 作ったもののうち、シーンがこのあと使うもの
    struct Arena {
        CameraFollow* cameraFollow = nullptr;
        Player* player = nullptr;
        PlayerController* controller = nullptr;
    };

    // needleCount は Bee の針の置き場の数 ステージかプレイヤーを作れなければ player は nullptr
    Arena Build(int needleCount);

    // ジャスト回避の文字と、そのあとの反撃の間の暗い幕を uiObject の子に足す 両方の場面で同じ
    void AddJustDodgeUi(GameObject* uiObject, Player* player);
}
