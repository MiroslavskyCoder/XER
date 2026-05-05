/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "content/device/bluetooth/bluetooth_device.h"

namespace device::bluetooth {

// Callback invoked when a device is discovered or updated during scanning.
using DeviceFoundCallback = std::function<void(const BluetoothDevice&)>;

// BluetoothAdapter wraps the system Bluetooth adapter.
// Uses the Linux BlueZ D-Bus / raw HCI socket interface under the hood.
class BluetoothAdapter {
public:
    static BluetoothAdapter& Instance();

    // Returns true if a Bluetooth adapter is present and powered on.
    bool IsAvailable() const;

    // Returns the local adapter address (e.g. "AA:BB:CC:DD:EE:FF").
    std::string GetAddress() const;

    // Start LE/BR+EDR inquiry scan.  Calls |callback| for every device found.
    // Scan runs synchronously for |duration_ms| milliseconds.
    bool StartScan(int duration_ms, DeviceFoundCallback callback);

    // Returns the devices discovered in the last scan.
    const std::vector<BluetoothDevice>& GetDiscoveredDevices() const;

    // Returns currently paired devices (read from BlueZ dbus or /var/lib/bluetooth).
    std::vector<BluetoothDevice> GetPairedDevices() const;

    // Initiate pairing with a device by address.
    bool Pair(const std::string& address, std::string* error = nullptr);

    // Disconnect a connected device.
    bool Disconnect(const std::string& address);

private:
    BluetoothAdapter() = default;
    std::vector<BluetoothDevice> discovered_;
};

}  // namespace device::bluetooth
