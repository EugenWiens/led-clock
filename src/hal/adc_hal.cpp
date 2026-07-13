#include "adc_hal.h"
#include "esp_adc/adc_oneshot.h"

void EspAdcHal::init() {
    adc_oneshot_unit_init_cfg_t init_cfg{};
    init_cfg.unit_id  = ADC_UNIT_1;
    init_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    adc_oneshot_new_unit(&init_cfg, &m_handle);

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,      // 0–3.1 V input range
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12-bit
    };
    // GPIO2 = ADC_UNIT_1 / ADC_CHANNEL_2 on ESP32-C6
    adc_oneshot_config_channel(m_handle, ADC_CHANNEL_2, &chan_cfg);
}

int EspAdcHal::read() {
    int raw = 0;
    adc_oneshot_read(m_handle, ADC_CHANNEL_2, &raw);
    return raw;
}
