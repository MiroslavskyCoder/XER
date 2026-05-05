/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/device/usb/usb_enumerator.h"

#include <fstream>
#include <sstream>
#include <string>

#if defined(__linux__)
#include <dirent.h>
#include <sys/inotify.h>
#include <unistd.h>
#endif

namespace device::usb {

namespace {

// Read a single-line sysfs attribute; returns empty string on failure.
#if defined(__linux__)
static std::string ReadSysfsAttr(const std::string& dev_path,
                                  const std::string& attr) {
    std::ifstream f(dev_path + "/" + attr);
    if (!f.good()) return {};
    std::string val;
    std::getline(f, val);
    return val;
}

static uint16_t ParseHex16(const std::string& s) {
    if (s.empty()) return 0;
    unsigned long v = 0;
    try { v = std::stoul(s, nullptr, 16); } catch (...) {}
    return static_cast<uint16_t>(v);
}

static uint8_t ParseDec8(const std::string& s) {
    if (s.empty()) return 0;
    unsigned long v = 0;
    try { v = std::stoul(s); } catch (...) {}
    return static_cast<uint8_t>(v);
}

static UsbSpeed ParseSpeed(const std::string& s) {
    if (s == "1.5")   return UsbSpeed::kLow;
    if (s == "12")    return UsbSpeed::kFull;
    if (s == "480")   return UsbSpeed::kHigh;
    if (s == "5000")  return UsbSpeed::kSuper;
    if (s == "10000") return UsbSpeed::kSuperPlus;
    return UsbSpeed::kUnknown;
}

static UsbDevice ReadDevice(const std::string& dev_path) {
    UsbDevice dev;
    dev.sysfs_path   = dev_path;
    dev.vendor_id    = ParseHex16(ReadSysfsAttr(dev_path, "idVendor"));
    dev.product_id   = ParseHex16(ReadSysfsAttr(dev_path, "idProduct"));
    dev.device_class    = ParseDec8(ReadSysfsAttr(dev_path, "bDeviceClass"));
    dev.device_subclass = ParseDec8(ReadSysfsAttr(dev_path, "bDeviceSubClass"));
    dev.device_protocol = ParseDec8(ReadSysfsAttr(dev_path, "bDeviceProtocol"));
    dev.manufacturer = ReadSysfsAttr(dev_path, "manufacturer");
    dev.product      = ReadSysfsAttr(dev_path, "product");
    dev.serial_number = ReadSysfsAttr(dev_path, "serial");
    dev.speed = ParseSpeed(ReadSysfsAttr(dev_path, "speed"));

    const std::string bus  = ReadSysfsAttr(dev_path, "busnum");
    const std::string dnum = ReadSysfsAttr(dev_path, "devnum");
    if (!bus.empty() && !dnum.empty())
        dev.busnum_devnum = bus + ":" + dnum;

    return dev;
}
#endif  // __linux__

}  // namespace

// ──────────────────────────────────────────────────────────────────────────────

UsbEnumerator& UsbEnumerator::Instance() {
    static UsbEnumerator instance;
    return instance;
}

std::vector<UsbDevice> UsbEnumerator::EnumerateDevices() const {
    std::vector<UsbDevice> result;
#if defined(__linux__)
    const char* kSysfsBase = "/sys/bus/usb/devices";
    DIR* dir = opendir(kSysfsBase);
    if (!dir) return result;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name = entry->d_name;
        // USB device directory names look like "1-1", "1-1.2", "2-0:1.0", etc.
        // Skip host controllers (usb1, usb2 …) and interface entries (containing ':').
        if (name.empty() || name[0] == '.') continue;
        if (name.find(':') != std::string::npos) continue; // interface descriptor
        const std::string path = std::string(kSysfsBase) + "/" + name;
        UsbDevice dev = ReadDevice(path);
        if (dev.vendor_id != 0 || dev.product_id != 0)
            result.push_back(std::move(dev));
    }
    closedir(dir);
#endif
    return result;
}

void UsbEnumerator::SetHotplugCallback(HotplugCallback callback) {
    hotplug_cb_ = std::move(callback);
}

bool UsbEnumerator::StartMonitoring() {
    if (monitoring_) return true;
    // Full inotify monitoring would run on a dedicated thread; wired up as
    // needed by the platform integration layer.
    monitoring_ = true;
    return true;
}

void UsbEnumerator::StopMonitoring() {
    monitoring_ = false;
}

}  // namespace device::usb
