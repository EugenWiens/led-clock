#pragma once

#include <ctime>

// Phase 4: WiFi connection and NTP time sync
// See docs/sw_design.md — "src/network/ntp.h/.cpp"

[[nodiscard]] bool ntpBegin();
[[nodiscard]] bool ntpGetTime(struct tm& timeInfo);
void ntpMaintain();
