// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

class IAdcHal {
public:
    virtual ~IAdcHal() = default;
    virtual void init() = 0;
    virtual int read() = 0;
};
