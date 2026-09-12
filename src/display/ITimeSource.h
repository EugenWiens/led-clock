// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <ctime>

class ITimeSource {
public:
    virtual ~ITimeSource() = default;
    [[nodiscard]] virtual bool getTime(struct tm& timeInfo) = 0;
};