#pragma once
#include "DxLib.h"
#include <string>

/// <summary>
/// 音声データクラス
/// UnityのAudioClipに対応
/// DxLibのサウンドハンドルをラップする
/// </summary>
class AudioClip {
public:
    /// <summary>DxLibサウンドハンドル</summary>
    int handle = -1;

    /// <summary>再生時間（秒）</summary>
    float length = 0.0f;

    /// <summary>クリップ名（デバッグ用）</summary>
    std::string name = "";

public:
    AudioClip() = default;

    ~AudioClip() {
        Unload();
    }

    /// <summary>
    /// 音声ファイルを読み込む
    /// </summary>
    /// <param name="filePath">ファイルパス（例："SE\\Player\\guard_success"）</param>
    /// <param name="is3D">true なら3Dサウンドとして読み込む</param>
    /// <returns>成功なら true</returns>
    bool Load(const char* filePath, bool is3D = false) {
        Unload();

        // 3Dサウンドフラグを設定してから読み込む
        SetCreate3DSoundFlag(is3D ? TRUE : FALSE);
        handle = LoadSoundMem(filePath);
        SetCreate3DSoundFlag(FALSE);

        if (handle == -1) {
            return false;
        }

        // 再生時間を取得（ミリ秒→秒に変換）
        int totalTimeMs = GetSoundTotalTime(handle);
        if (totalTimeMs > 0) {
            length = totalTimeMs / 1000.0f;
        }

        name = filePath;
        return true;
    }

    /// <summary>
    /// サウンドハンドルを解放する
    /// </summary>
    void Unload() {
        if (handle != -1) {
            DeleteSoundMem(handle);
            handle = -1;
            length = 0.0f;
        }
    }

    /// <summary>
    /// 読み込み済みかどうか
    /// </summary>
    bool IsLoaded() const { return handle != -1; }
};
