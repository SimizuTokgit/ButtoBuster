#pragma once

/// <summary>
/// シーン基底クラス
/// 各シーン（TitleScene, GameScene, ResultScene等）はこれを継承する
/// </summary>
class SceneBase {
public:
    virtual ~SceneBase() = default;

    /// <summary>
    /// シーン読み込み時に呼ばれる（GameObjectの作成・設定を行う）
    /// </summary>
    virtual bool OnLoad() = 0;

    /// <summary>
    /// シーン破棄時に呼ばれる（シーン固有の後処理）
    /// </summary>
    virtual void OnUnload() {}
};
