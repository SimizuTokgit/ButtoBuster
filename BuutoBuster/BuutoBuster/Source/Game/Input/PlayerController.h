#pragma once
#include "MonoBehaviour.h"
#include "InputInfo.h"
#include "TargetLock.h"

class Player;
class Character;
class CameraFollow;

// 人の操作を InputInfo にして Player に渡す
// 同時押しの判定はここで行い、決まった技だけを渡す
// 敵も同じ InputInfo を受け取るので、技の処理は人と AI で共通になる
//
// 視点を回す操作とロックオンもここで読む 人の操作を読むのはこのクラスだけにしておく
class PlayerController : public MonoBehaviour {
private:
    // 最初のボタンから何フレーム待って技を決めるか
    // 押した瞬間に決めると、指のずれで単発技が暴発する
    // 3 では短すぎて、クリックとキーの組み合わせが揃わなかった
    static constexpr int COMBINE_WAIT_FRAMES = 6;

    // 倒した向きがこれより変わったら、進む向きをカメラの今の向きで決め直す 度
    static constexpr float MOVE_REBASE_DEGREE = 30.0f;

    static constexpr int ATTACK_BIT = 1 << 0;
    static constexpr int GUARD_BIT = 1 << 1;
    static constexpr int JUMP_BIT = 1 << 2;

    // 視点を回す速さ 度/秒 マウスは 1 ドット動かしたときの度
    static constexpr float STICK_YAW_SPEED = 180.0f;
    static constexpr float STICK_PITCH_SPEED = 100.0f;
    static constexpr float KEY_YAW_SPEED = 150.0f;
    static constexpr float MOUSE_SENSITIVITY = 0.15f;

    // ロックオン中に右スティックをこれより倒したら、隣の敵へ移す
    static constexpr float FLICK_THRESHOLD = 0.7f;

    // ロックオン中にマウスを素早くこれだけ動かしたら、隣の敵へ移す
    static constexpr float MOUSE_SWITCH_DISTANCE = 150.0f;

    // マウスの動きを忘れていく速さ ゆっくり動かしただけでは移らないように
    static constexpr float MOUSE_SWITCH_DECAY = 8.0f;

    Player* _player = nullptr;
    CameraFollow* _camera = nullptr;
    TargetLock _targetLock;

    bool _isWaiting = false;
    int _waitedFrames = 0;
    int _collectedButtons = 0;

    bool _wasStickFlicked = false;
    float _mouseSwitchAmount = 0.0f;

    // 進む向きを決めたときのカメラの向き
    // カメラが背中へ回り込む間も、押し続けている間は同じ向きへ走り続ける
    // その場で決め直すと、横を押し続けたときに円を描いて走ってしまう
    bool _hasMoveBasis = false;
    float _moveBasisDegree = 0.0f;
    VECTOR _moveForward = VGet(0.0f, 0.0f, 1.0f);
    VECTOR _moveRight = VGet(1.0f, 0.0f, 0.0f);

    // このフレームに手で視点を回したか 回したら進む向きも決め直す
    bool _hasViewInput = false;

public:
    // 操作を受け付けない ゲームオーバーの後など
    bool isInputLocked = false;

    ~PlayerController() override;

    void Start() override;
    void Update(float deltaTime) override;

    // 移動の向きをカメラから見た向きに直すため、視点を回すために使う
    void SetCamera(CameraFollow* camera) { _camera = camera; }

    // ロックオンしている相手 いなければ nullptr
    const Character* GetLockOnTarget() const { return _targetLock.GetTarget(); }

private:
    Technique UpdateCombination();
    int ReadPressedButtons() const;
    int ReadHeldButtons() const;
    VECTOR ReadMove();

    void UpdateLockOn();
    void UpdateView();
    VECTOR GetViewForward() const;
    VECTOR GetLockOnDirection() const;

    static Technique Resolve(int buttons);
    static int CountBits(int bits);
};
