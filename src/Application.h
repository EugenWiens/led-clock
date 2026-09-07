// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include "ble/Bluetooth.h"
#include "ble/SwitchBot.h"
#include "display/Display.h"
#include "hal/EspAdcHal.h"
#include "hal/EspLedStripHal.h"
#include "network/Ntp.h"

class Application final {
public:
    void run();

private:
    static uint64_t nowMs();

    EspLedStripHal m_ledHal;
    EspAdcHal m_adcHal;
    Bluetooth m_bluetooth;
    SwitchBot m_switchBot{m_bluetooth};
    Ntp m_ntp;
    Display m_display{m_ledHal, m_adcHal, m_ntp, m_switchBot};
};
