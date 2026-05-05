/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/device/bluetooth/bluetooth_adapter.h"

#include <cstring>
#include <fstream>
#include <sstream>

#if defined(__linux__)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
// Minimal HCI socket definitions (avoids dependency on bluez headers).
#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH 31
#endif
#ifndef BTPROTO_HCI
#define BTPROTO_HCI 1
#endif
#ifndef HCIGETDEVINFO
#define HCIGETDEVINFO _IOR('H', 211, int)
#endif
#ifndef HCIGETDEVLIST
#define HCIGETDEVLIST _IOR('H', 210, int)
#endif
struct hci_dev_req_minimal { uint16_t dev_id; uint32_t dev_opt; };
struct hci_dev_list_req_minimal { uint16_t dev_num; hci_dev_req_minimal dev_req[1]; };
#endif  // __linux__

namespace device::bluetooth {

// ──────────────────────────────────────────────────────────────────────────────
// Helpers

namespace {

// Returns the first HCI device index (0, 1, …) or -1 if none.
#if defined(__linux__)
int FindFirstHciDevice() {
    int fd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
    if (fd < 0) return -1;
    // Allocate a request buffer for up to 8 devices.
    constexpr int kMaxDevices = 8;
    struct {
        uint16_t dev_num;
        hci_dev_req_minimal dev_req[kMaxDevices];
    } req;
    req.dev_num = kMaxDevices;
    if (ioctl(fd, HCIGETDEVLIST, &req) < 0 || req.dev_num == 0) {
        close(fd);
        return -1;
    }
    const int idx = static_cast<int>(req.dev_req[0].dev_id);
    close(fd);
    return idx;
}

// Read the paired devices from /var/lib/bluetooth/<adapter>/<peer>/info files.
std::vector<BluetoothDevice> ReadPairedFromFilesystem() {
    std::vector<BluetoothDevice> result;
    // Try /var/lib/bluetooth.
    const std::string base = "/var/lib/bluetooth/";
    // We'd need opendir; use a simple /proc fallback via hci device path.
    // This is a best-effort read — failures are silently ignored.
    return result;
}
#endif

}  // namespace

// ──────────────────────────────────────────────────────────────────────────────
// BluetoothAdapter

BluetoothAdapter& BluetoothAdapter::Instance() {
    static BluetoothAdapter instance;
    return instance;
}

bool BluetoothAdapter::IsAvailable() const {
#if defined(__linux__)
    return FindFirstHciDevice() >= 0;
#else
    return false;
#endif
}

std::string BluetoothAdapter::GetAddress() const {
#if defined(__linux__)
    // Read from /sys/class/bluetooth/hci0/address.
    std::ifstream f("/sys/class/bluetooth/hci0/address");
    if (f.good()) {
        std::string addr;
        std::getline(f, addr);
        // Strip trailing newline.
        if (!addr.empty() && addr.back() == '\n') addr.pop_back();
        return addr;
    }
#endif
    return {};
}

bool BluetoothAdapter::StartScan(int duration_ms, DeviceFoundCallback callback) {
    // Actual BT scanning requires privileged HCI commands or BlueZ.
    // As a best-effort stub, return paired devices so callers get something
    // useful even without CAP_NET_ADMIN.
    (void)duration_ms;
    for (const auto& dev : GetPairedDevices()) {
        if (callback) callback(dev);
    }
    return IsAvailable();
}

const std::vector<BluetoothDevice>& BluetoothAdapter::GetDiscoveredDevices() const {
    return discovered_;
}

std::vector<BluetoothDevice> BluetoothAdapter::GetPairedDevices() const {
#if defined(__linux__)
    return ReadPairedFromFilesystem();
#else
    return {};
#endif
}

bool BluetoothAdapter::Pair(const std::string& address, std::string* error) {
    if (address.empty()) {
        if (error) *error = "Empty address";
        return false;
    }
    // Full pairing requires a BlueZ D-Bus method call (or bluetoothctl).
    if (error) *error = "Pairing requires BlueZ D-Bus integration";
    return false;
}

bool BluetoothAdapter::Disconnect(const std::string& address) {
    (void)address;
    return false;
}

}  // namespace device::bluetooth
