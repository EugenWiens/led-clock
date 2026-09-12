// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include "ble/SwitchBotData.h"

class ISensorSource {
public:
    virtual ~ISensorSource() = default;
    [[nodiscard]] virtual bool getData(SwitchBotData& data) = 0;
};