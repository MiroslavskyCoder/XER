/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "content/device/bluetooth/bluetooth_device.h"

namespace device::bluetooth {

// A single GATT characteristic.
struct GattCharacteristic {
    std::string uuid;
    std::string handle_hex;  // e.g. "0x0025"
    std::vector<uint8_t> value;
    bool readable   = false;
    bool writable   = false;
    bool notifiable = false;
};

// A single GATT service.
struct GattService {
    std::string uuid;
    std::string handle_hex;
    std::vector<GattCharacteristic> characteristics;
};

// Callback types.
using GattDiscoveryCallback =
    std::function<void(const std::string& address,
                       const std::vector<GattService>& services,
                       const std::string& error)>;

using GattNotifyCallback =
    std::function<void(const std::string& char_uuid,
                       const std::vector<uint8_t>& value)>;

// BluetoothGattClient discovers services and reads/writes GATT characteristics.
// On Linux uses BlueZ D-Bus (org.bluez.GattManager1) when available, falls
// back to raw HCI ACL frames.  Async operations run on IOThreadPool.
class BluetoothGattClient {
public:
    static BluetoothGattClient& Instance();

    // Discover all GATT services and characteristics for |address|.
    // The result is delivered asynchronously via |callback| on the thread pool.
    void DiscoverServicesAsync(const std::string& address,
                                GattDiscoveryCallback callback);

    // Synchronously read characteristic |char_uuid| of device |address|.
    bool ReadCharacteristic(const std::string& address,
                             const std::string& char_uuid,
                             std::vector<uint8_t>* value_out,
                             std::string* error = nullptr);

    // Write |data| to characteristic |char_uuid| of device |address|.
    bool WriteCharacteristic(const std::string& address,
                              const std::string& char_uuid,
                              const std::vector<uint8_t>& data,
                              std::string* error = nullptr);

    // Subscribe to notifications for |char_uuid|.
    bool Subscribe(const std::string& address,
                   const std::string& char_uuid,
                   GattNotifyCallback callback);

    void Unsubscribe(const std::string& address, const std::string& char_uuid);

private:
    BluetoothGattClient() = default;

    // Runs gatttool or bluetoothctl to perform GATT operations when no D-Bus.
    std::string RunGatttool(const std::string& address,
                             const std::string& args) const;
};

}  // namespace device::bluetooth
