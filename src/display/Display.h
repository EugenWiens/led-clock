// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include "ble/SwitchBot.h"
#include "network/Ntp.h"
#include "Matrix.h"
#include "Renderer.h"
#include "DisplayState.h"

class Display {
public:
    Display(ILedHal& ledHal, IAdcHal& adcHal, Ntp& ntp, SwitchBot& switchBot);

    void init(uint64_t nowMs);
    void update(uint64_t nowMs);

private:
    Matrix m_matrix;
    Renderer m_renderer;
    Ntp& m_ntp;
    SwitchBot& m_switchBot;
    DisplayState m_state{DisplayState::SHOW_CLOCK};
    uint64_t m_stateEnteredAt{0};
    uint64_t m_lastColonToggle{0};
    bool m_colonOn{true};
};
