// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <cstdint>
#include <ctime>

#ifndef NATIVE_ENV

#include "esp_event.h"
#endif

class Ntp {
public:
    [[nodiscard]] bool init();
    [[nodiscard]] bool getTime(struct tm& timeInfo);
    void maintain();

private:
#ifndef NATIVE_ENV
    static constexpr char TAG[] = "ntp";
    static constexpr time_t MIN_VALID_EPOCH = 1577836800;

    static void wifiEventHandler(void* argument, esp_event_base_t eventBase, int32_t eventId,
                                 void* eventData);
    static uint64_t nowMs();
    static bool copyWifiValue(uint8_t* destination, size_t capacity, const char* source);
    static bool readSystemTime(struct tm& timeInfo);

    bool initWifi();
    void requestWifiConnection(uint64_t currentMs);
    bool configTzTime();
    bool startSntp();
    bool pollForSync();
    bool getLocalTime(struct tm& timeInfo, uint32_t timeoutMs);
    bool waitForWifi(uint32_t timeoutMs);

    std::atomic_bool m_wifiHasIp{false};
    bool m_wifiInitialized{false};
    bool m_wifiStarted{false};
    bool m_sntpInitialized{false};
    std::atomic_bool m_syncPending{false};
    std::atomic_bool m_resyncRequired{true};
    bool m_timeValid{false};
    uint64_t m_nextReconnectAtMs{0};
    uint64_t m_nextSyncAttemptAtMs{0};
#endif
};
