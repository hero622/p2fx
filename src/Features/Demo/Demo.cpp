#include "Demo.hpp"

#include "P2FX.hpp"

#include <cstdint>

int32_t Demo::LastTick() {
	return (!this->messageTicks.empty())
		? this->messageTicks.back()
		: this->playbackTicks;
}
float Demo::IntervalPerTick() {
	return (this->playbackTicks != 0)
		? this->playbackTime / this->playbackTicks
		: 1 / p2fx.game->Tickrate();
}
float Demo::Tickrate() {
	return (this->playbackTime != 0)
		? this->playbackTicks / this->playbackTime
		: p2fx.game->Tickrate();
}
