#pragma once

#include <cstddef>
#include <cstdint>

struct ble_gap_event;

struct BluetoothAdvertisement {
    uint8_t address[6]{};
    const uint8_t* data{nullptr};
    size_t dataLength{0};
    uint64_t timestampMs{0};
};

using BluetoothAdvertisementHandler = void (*)(const BluetoothAdvertisement& advertisement, void* context);

class Bluetooth {
public:
    void setAdvertisementHandler(BluetoothAdvertisementHandler handler, void* context);
    void init();

private:
    void notify(const uint8_t address[6], const uint8_t* data, size_t dataLength, uint64_t timestampMs);

    BluetoothAdvertisementHandler m_handler{nullptr};
    void* m_context{nullptr};

    friend int bluetoothGapEventCallback(struct ble_gap_event* event, void* argument);
};
