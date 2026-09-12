// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#include "Display.h"

#include <cmath>
#include <ctime>

Display::Display(ILedHal& ledHal, IAdcHal& adcHal, ITimeSource& timeSource,
                                 ISensorSource& sensorSource)
        : m_matrix{ledHal, adcHal},
            m_renderer{m_matrix},
            m_timeSource{timeSource},
            m_sensorSource{sensorSource} {}

void Display::init(uint64_t nowMs) {
    m_matrix.init();
    m_state = DisplayState::SHOW_CLOCK;
    m_stateEnteredAt = nowMs;
    m_lastColonToggle = nowMs;
    m_colonOn = true;
}

void Display::update(uint64_t nowMs) {
    if (nowMs - m_lastColonToggle >= COLON_TOGGLE_MS) {
        m_colonOn = !m_colonOn;
        m_lastColonToggle = nowMs;
    }

    if (m_state == DisplayState::SHOW_CLOCK && nowMs - m_stateEnteredAt >= CLOCK_DISPLAY_MS) {
        m_state = DisplayState::SHOW_TEMP;
        m_stateEnteredAt = nowMs;
    } else if (m_state == DisplayState::SHOW_TEMP && nowMs - m_stateEnteredAt >= TEMP_DISPLAY_MS) {
        m_state = DisplayState::SHOW_CLOCK;
        m_stateEnteredAt = nowMs;
    }

    if (m_state == DisplayState::SHOW_CLOCK) {
        struct tm timeInfo{};
        if (m_timeSource.getTime(timeInfo)) {
            m_renderer.renderClock(static_cast<uint8_t>(timeInfo.tm_hour),
                                   static_cast<uint8_t>(timeInfo.tm_min), m_colonOn);
        }
    } else {
        SwitchBotData sensorData{};
        if (m_sensorSource.getData(sensorData)) {
            m_renderer.renderTemp(sensorData.tempC);
        } else {
            m_renderer.renderTemp(NAN);
        }
    }

    m_matrix.updateBrightness(nowMs);
    m_matrix.show();
}
