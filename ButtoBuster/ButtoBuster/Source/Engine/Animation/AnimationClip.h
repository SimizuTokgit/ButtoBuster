#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "DxLib.h"

/// <summary>
/// アニメーションイベント
/// AnimationClipの特定タイミングで発火するコールバック
/// </summary>
struct AnimationEvent {
    /// <summary>発火するアニメーション時間</summary>
    float time = 0.0f;

    /// <summary>発火時に実行するコールバック</summary>
    std::function<void()> callback;
};

/// <summary>
/// アニメーションクリップ情報を保持するクラス
/// </summary>
class AnimationClip {
public:
    int Handle;               // DxLibのアニメーションハンドル
    std::string Name;
    float Speed;              // 再生速度（1秒あたりのフレーム数）
    float TotalTime;          // アニメーションの総時間
    bool IsLoop;              // ループアニメーションかどうか

    /// <summary>登録されたアニメーションイベント</summary>
    std::vector<AnimationEvent> events;

public:
    AnimationClip()
        : Name("")
        , Handle(-1)
        , Speed(0.0f)
        , TotalTime(0.0f)
        , IsLoop(false)
    {
    }

    bool Load(const char* animPath, const std::string& animName, float animSpeed, bool loop) {
        Handle = LoadSharedHandle(animPath);
        if (Handle == -1) return false;

        Name = animName;
        Speed = animSpeed;
        IsLoop = loop;

        // 長さは読み込んだ時点で持っておく まだ一度も再生していないアニメでも長さが分かるように
        TotalTime = MV1GetAnimTotalTime(Handle, 0);

        return true;
    }

    /// <summary>
    /// アニメーションファイルのハンドルをパス単位で共有する
    ///
    /// 敵を大量に出す場合、同じ Anim_*.mv1 をインスタンスの数だけ
    /// MV1LoadModel するとロード時間もメモリも無駄になる。
    /// アニメーションは「再生元」として参照されるだけで
    /// インスタンスごとの状態を持たないため、1つを共有して問題ない。
    ///
    /// （キャラクター本体のモデルは各自がポーズを持つので共有できない。
    ///   そちらは Renderer::LoadDuplicate で複製する）
    /// </summary>
    static int LoadSharedHandle(const char* animPath) {
        // 関数内 static にして、初回アクセス時に生成されるようにする
        static std::unordered_map<std::string, int> cache;

        std::string key = animPath;

        auto it = cache.find(key);
        if (it != cache.end()) {
            return it->second;
        }

        int handle = MV1LoadModel(animPath);
        if (handle == -1) return -1;

        cache.emplace(key, handle);
        return handle;
    }

    /// <summary>
    /// アニメーションイベントを登録する
    /// </summary>
    /// <param name="time">発火するアニメーション時間</param>
    /// <param name="callback">実行するコールバック</param>
    void AddEvent(float time, std::function<void()> callback) {
        events.push_back({ time, std::move(callback) });
    }
};
