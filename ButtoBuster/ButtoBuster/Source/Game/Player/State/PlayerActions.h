#pragma once
#include "ICharacterState.h"

class Player;

// 状態をまたいで使う判断と手応え
namespace PlayerActions {
    // スティックが倒されているか
    bool HasMoveInput(const InputInfo& input);

    // 地上で技やガードを始める 始めたら true
    // 待機 移動 着地 ガード 回避のあとや、技の振り終わりなど、次の行動を受け付けるところから呼ぶ
    bool TryStart(Player& player, const ICharacterState<Player>* from, const InputInfo& input);

    // 空中で技を始める 始めたら true
    // 攻撃は空中の斬り、攻撃 + ガードは真下への叩きつけ、攻撃 + ジャンプは斬り上げ
    bool TryStartAir(Player& player, const ICharacterState<Player>* from, const InputInfo& input);

    // 動けない間 (振っている途中や寄っている間) に押された技を覚えておき、動けるようになったら出す
    struct QueuedTechnique {
        Technique technique = Technique::None;

        // 押された技を覚える 何も押していないフレームは、前に押した技を残す
        void Remember(const InputInfo& input);

        // 今の入力の技を、覚えた技に替えたもの 向きとガードは今の入力のまま
        InputInfo ToInput(const InputInfo& input) const;

        // 覚えた技を始める 地上なら TryStart、宙なら TryStartAir を通す 始めたら true
        // 覚えた技が無くても、地上ならガードを握っているかは見る
        bool TryStart(Player& player, const ICharacterState<Player>* from, const InputInfo& input) const;
    };

    // 寄る技 (AttackData の zoomPunch) が当たったときに、寄って画面を白く光らせる 0 なら何もしない
    // 寄る技ほど重いので、光も強くする
    void PlayZoomFlash(float zoomPunch);
}
