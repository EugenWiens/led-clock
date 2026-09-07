// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include "hal/IAdcHal.h"

class StubAdcHal final : public IAdcHal {
public:
    int value{2048};

    void init() override {}
    int read() override { return value; }
};
