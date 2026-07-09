#include "display/matrix.h"
#include "display/renderer.h"
#include "network/ntp.h"
#include "ble/switchbot.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <ctime>
#include <cmath>

enum DisplayState { SHOW_CLOCK, SHOW_TEMP };

static uint64_t ms_now() {
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

extern "C" void app_main() {

    DisplayState state        = SHOW_CLOCK;
    uint64_t stateEnteredAt   = ms_now();
    uint64_t lastColonToggle  = ms_now();
    bool colonOn              = true;

    for (;;) {
        const uint64_t now = ms_now();
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
    }
}
