// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#include "Application.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint64_t Application::nowMs() {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
}

void Application::run() {
    m_bluetooth.init();
    m_switchBot.init();
    (void)m_ntp.init();
    m_display.init(nowMs());

    for (;;) {
        const uint64_t now = nowMs();
        m_ntp.maintain();
        m_display.update(now);
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
    }
}
