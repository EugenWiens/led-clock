// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include "ISensorSource.h"
#include "ITimeSource.h"
#include "Matrix.h"
#include "Renderer.h"
#include "DisplayState.h"

class Display {
public:
    Display(ILedHal& ledHal, IAdcHal& adcHal, ITimeSource& timeSource, ISensorSource& sensorSource);

    void init(uint64_t nowMs);
    void update(uint64_t nowMs);

private:
    Matrix m_matrix;
    Renderer m_renderer;
    ITimeSource& m_timeSource;
    ISensorSource& m_sensorSource;
    DisplayState m_state{DisplayState::SHOW_CLOCK};
    uint64_t m_stateEnteredAt{0};
    uint64_t m_lastColonToggle{0};
    bool m_colonOn{true};
};
