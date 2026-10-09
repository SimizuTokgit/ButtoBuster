#include "SpecialGaugeObserver.h"
#include "Player.h"

void SpecialGaugeObserver::OnNotify(const ChainEvent& event) {
    if (!_player || event.type != ChainEvent::Type::Hit) return;
    _player->AddSpecialGauge(_player->data.specialGainPerChain);
}

void SpecialGaugeObserver::OnNotify(const WallBreakEvent& event) {
    if (!_player || event.isPlayer) return;
    _player->AddSpecialGauge(_player->data.specialGainPerWallBreak);
}
