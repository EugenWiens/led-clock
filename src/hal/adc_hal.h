#pragma once

/// Abstract ADC hardware interface — injected into Matrix via constructor.
class IAdcHal {
public:
    virtual ~IAdcHal() = default;
    virtual void init() = 0;
    virtual int read() = 0; ///< Returns a raw 12-bit sample: 0–4095.
};

#ifndef NATIVE_ENV
#include "esp_adc/adc_oneshot.h"

/// ESP-IDF adc_oneshot implementation for ADC_UNIT_1 / ADC_CHANNEL_2 (GPIO2).
class EspAdcHal final : public IAdcHal {
public:
    void init() override;
    int read() override;

private:
    adc_oneshot_unit_handle_t m_handle{};
};
#endif
