#include "display/display.h"
#include "ble/bluetooth.h"
#include "ble/switchbot.h"
#include "hal/led_hal.h"
#include "hal/adc_hal.h"
#include "network/ntp.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdint>

static uint64_t ms_now() {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
}

// Static objects avoid placing the ~960 B LED buffer on the app_main stack.
static EspLedStripHal s_ledHal;
static EspAdcHal s_adcHal;
static Bluetooth s_bluetooth;
static SwitchBot s_switchBot{s_bluetooth};
static Ntp s_ntp;
static Display s_display{s_ledHal, s_adcHal, s_ntp, s_switchBot};

extern "C" void app_main() {
    s_switchBot.init();
    s_bluetooth.init();
    (void)s_ntp.init();
    s_display.init(ms_now());

    for (;;) {
        const uint64_t now = ms_now();
        s_ntp.maintain();
        s_display.update(now);
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
    }
}
