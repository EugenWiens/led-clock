#include "display/matrix.h"
#include "hal/led_hal.h"
#include "hal/adc_hal.h"
// #include "display/renderer.h"   // Phase 3
// #include "network/ntp.h"         // Phase 4
// #include "ble/switchbot.h"        // Phase 5

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <ctime>
#include <cmath>

enum DisplayState { SHOW_CLOCK, SHOW_TEMP };

static uint64_t ms_now() {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
}

extern "C" void app_main() {
    // Static storage: keeps large objects (Matrix = ~960 B) off the task stack.
    static FastLedHal ledHal;
    static EspAdcHal  adcHal;
    static Matrix     matrix{ledHal, adcHal};

    matrix.init();

    DisplayState state       = SHOW_CLOCK;
    uint64_t stateEnteredAt  = ms_now();
    uint64_t lastColonToggle = ms_now();
    bool colonOn             = true;

    for (;;) {
        const uint64_t now = ms_now();
        matrix.updateBrightness(now);
        matrix.show();
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
    }
}
