#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "AudioClip.h"

class AudioSource;
class GameObject;

/// <summary>
/// サウンド一元管理シングルトンクラス
/// 起動時に Data/Sound/SE, Data/Sound/BGM 配下を再帰走査して
/// フォルダ込みの相対パス（拡張子なし）をキーに AudioClip を保持する。
/// 内部に AudioSource を複数 AddComponent したプールを持ち、
/// PlaySE / PlayBGM で未使用のものから再生する（Unity 時代の設計を踏襲）。
/// </summary>
class SoundManager {
public:
    /// <summary>シングルトンインスタンスを取得</summary>
    static SoundManager& Instance() {
        static SoundManager instance;
        return instance;
    }

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /// <summary>
    /// サウンド専用 GameObject と AudioSource プールを構築し、
    /// Data/Sound/SE, Data/Sound/BGM 配下を一括ロードする。
    /// DxLib_Init() の後に 1 度だけ呼ぶこと。
    /// </summary>
    void Init();

    /// <summary>
    /// AudioSource / AudioClip を全解放する。
    /// DxLib_End() の前に呼ぶこと。
    /// </summary>
    void Terminate();

    /// <summary>
    /// 名前指定で SE を 2D 再生する。
    /// 同一 SE は INTERVAL 以内の連続呼び出しを無視する。
    /// プール内の未使用 AudioSource がなければ再生されない。
    /// </summary>
    /// <param name="seName">SE 名（Data/Sound/SE からの拡張子なしの相対パス 例 Player/guard_success）</param>
    /// <param name="volumeScale">音量スケール (0.0～1.0)</param>
    void PlaySE(const std::string& seName, float volumeScale = 1.0f);

    /// <summary>
    /// 名前指定で BGM をループ再生する。
    /// fadeInDuration > 0 を指定するとフェードイン付きで再生する。
    /// </summary>
    /// <param name="bgmName">BGM 名（拡張子なしのファイル名）</param>
    /// <param name="fadeInDuration">フェードイン秒数（0 で即時最大音量）</param>
    void PlayBGM(const std::string& bgmName, float fadeInDuration = 0.0f);

    /// <summary>
    /// BGMフォルダのクリップをループさせずに一度だけ鳴らす（ジングル用）
    ///
    /// クリア音やゲームオーバー音は BGM と同じフォルダに置かれているが、
    /// PlayBGM はループ再生なのでそのままでは鳴り続けてしまう
    /// </summary>
    /// <param name="jingleName">拡張子なしのファイル名</param>
    /// <param name="volumeScale">音量倍率</param>
    void PlayJingle(const std::string& jingleName, float volumeScale = 1.0f);

    /// <summary>
    /// 名前指定で BGM を停止する。
    /// fadeOutDuration > 0 でフェードアウトしてから停止する。
    /// </summary>
    void StopBGM(const std::string& bgmName, float fadeOutDuration = 0.0f);

    /// <summary>
    /// 全 BGM を停止する。
    /// fadeOutDuration > 0 でフェードアウトしてから停止する。
    /// </summary>
    void StopAllBGM(float fadeOutDuration = 0.0f);

    /// <summary>
    /// 現在の BGM をフェードアウトしながら、新しい BGM をフェードインで切り替える。
    /// 同じ BGM が既に再生中の場合は何もしない。
    /// </summary>
    /// <param name="bgmName">切り替え先 BGM 名</param>
    /// <param name="duration">クロスフェード秒数（フェードアウトとフェードインの両方に使用）</param>
    void CrossfadeBGM(const std::string& bgmName, float duration);

    /// <summary>
    /// フェード処理を進める。System の MainLoop から毎フレーム呼ぶこと。
    /// </summary>
    void Update(float deltaTime);

    /// <summary>SE 用 AudioClip を取得（3D 再生等で外部の AudioSource に渡したい場合）</summary>
    AudioClip* GetSEClip(const std::string& seName);

    /// <summary>BGM 用 AudioClip を取得</summary>
    AudioClip* GetBGMClip(const std::string& bgmName);

    /// <summary>SE マスター音量 (0.0～1.0)</summary>
    void SetSEVolume(float v);

    /// <summary>BGM マスター音量 (0.0～1.0)</summary>
    void SetBGMVolume(float v);

    float GetSEVolume() const { return _seVolume; }
    float GetBGMVolume() const { return _bgmVolume; }

private:
    // 作るのと消すのは cpp に書く
    // unique_ptr<GameObject> を消すには GameObject の中身が見えている必要があり、
    // ここで書くと、この h を読むすべてのファイルに GameObject.h が要ることになる
    // 作る側も、途中で失敗したときに作りかけのメンバを消す処理が入るので同じ
    SoundManager();
    ~SoundManager();

    using ClipMap = std::unordered_map<std::string, std::unique_ptr<AudioClip>>;

    /// <summary>
    /// フォルダを再帰走査して対応拡張子の音声を一括ロード
    /// キーはフォルダを含めた相対パス 例 Goblin/VO_dmg_00
    /// 敵ごとに同じファイル名が並んでいるので、ファイル名だけだと先勝ちで潰れる
    /// </summary>
    void LoadFolder(const std::string& folderPath, const std::string& keyPrefix, ClipMap& target, bool makeGroups);

    /// <summary>
    /// 名前から実際に鳴らすクリップ名を決める
    /// 末尾の _00 _01 を省いた名前なら、その中からランダムに1つ選ぶ
    /// </summary>
    const std::string* ResolveSEName(const std::string& seName) const;

    /// <summary>未使用の SE 用 AudioSource を取得（全て再生中なら nullptr）</summary>
    AudioSource* GetUnusedSESource();

    /// <summary>未使用の BGM 用 AudioSource を取得</summary>
    AudioSource* GetUnusedBGMSource();

    static float ClampVolume(float v) {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    // AudioSource プールのサイズ
    // BGM はクロスフェードで同時に 2 本鳴らせるよう 2 にする
    static constexpr int SE_SOURCE_COUNT = 20;
    static constexpr int BGM_SOURCE_COUNT = 2;

    // 同名 SE の連続再生抑制間隔（秒）
    static constexpr float INTERVAL = 0.2f;

    // 全 AudioSource を所有する専用 GameObject（SoundManager 専用、Scene には属さない）
    std::unique_ptr<GameObject> _soundGameObject;

    // プール内のポインタ参照（所有は _soundGameObject 側）
    std::vector<AudioSource*> _seSources;
    std::vector<AudioSource*> _bgmSources;

    ClipMap _seClips;
    ClipMap _bgmClips;

    // 番号違いの同じ音をまとめたもの 例 Goblin/VO_dmg → Goblin/VO_dmg_00 〜 _04
    std::unordered_map<std::string, std::vector<std::string>> _seGroups;

    // クリップごとの最終再生時刻（秒）
    std::unordered_map<std::string, float> _lastPlayedTimes;

    float _seVolume = 1.0f;
    float _bgmVolume = 1.0f;

    bool _initialized = false;

    /// <summary>BGM ソースに対する進行中のフェード情報</summary>
    struct BGMFade {
        AudioSource* source = nullptr;
        float startVolume = 0.0f;
        float targetVolume = 0.0f;
        float elapsed = 0.0f;
        float duration = 0.0f;
        bool stopOnComplete = false;
    };
    std::vector<BGMFade> _bgmFades;

    /// <summary>指定ソースに対するフェードを開始（既存のフェードは打ち切る）</summary>
    void StartBGMFade(AudioSource* src, float startVol, float targetVol,
                     float duration, bool stopOnComplete);

    /// <summary>指定ソースに対する既存フェードを破棄</summary>
    void RemoveBGMFade(AudioSource* src);
};
