#include "display/matrix.h"
#include "display/renderer.h"
#include "hal/led_hal.h"
#include "hal/adc_hal.h"
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

// Static objects avoid placing the ~960 B LED buffer on the app_main stack.
static EspLedStripHal s_ledHal;
static EspAdcHal      s_adcHal;
static Matrix         s_matrix{s_ledHal, s_adcHal};
static Renderer       s_renderer{s_matrix};

static DisplayState s_state{SHOW_CLOCK};
static uint64_t     s_stateEnteredAt{0};
static uint64_t     s_lastColonToggle{0};
static bool         s_colonOn{true};

extern "C" void app_main() {
    s_matrix.init();
    s_stateEnteredAt  = ms_now();
    s_lastColonToggle = ms_now();

    for (;;) {
        const uint64_t now = ms_now();
        s_matrix.updateBrightness(now);
        s_matrix.show();
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
    }
}
