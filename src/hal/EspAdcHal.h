// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#ifndef NATIVE_ENV

#include "esp_adc/adc_oneshot.h"

#include "IAdcHal.h"

class EspAdcHal final : public IAdcHal {
public:
    void init() override;
    int read() override;

private:
    adc_oneshot_unit_handle_t m_handle{};
};
#endif
