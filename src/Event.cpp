#include "Event.hpp"

#include <map>

P2fxInitHandler::P2fxInitHandler(std::function<void()> cb)
	: cb(cb) {
		P2fxInitHandler::GetHandlers().push_back(this);
}

void P2fxInitHandler::RunAll() {
	for (auto h : P2fxInitHandler::GetHandlers()) {
		h->cb();
	}
}

std::vector<P2fxInitHandler *> &P2fxInitHandler::GetHandlers() {
	static std::vector<P2fxInitHandler *> handlers;
	return handlers;
}
