#pragma once

#include <ctime>

// Phase 4: WiFi connection and NTP time sync
// See docs/sw_design.md — "src/network/ntp.h/.cpp"

class Ntp {
public:
	[[nodiscard]] bool init();
	[[nodiscard]] bool getTime(struct tm& timeInfo);
	void maintain();
};
