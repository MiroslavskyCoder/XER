/**
 * This file is part of XER, Device module.
 * Copyright (C) 2026 Yoshi A.
 *
 * Exposes the device subsystems (Bluetooth, USB, Wi-Fi, VR, Cast) as a V8
 * JavaScript module so FlowScript / XER JS code can access hardware.
 *
 * Usage example (JS):
 *   const Device = require('Device');
 *
 *   // Bluetooth scan
 *   const devices = Device.bluetooth.scan(3000);
 *
 *   // USB enumeration
 *   const usb_list = Device.usb.enumerate();
 *
 *   // Wi-Fi
 *   const networks = Device.wifi.scan("wlan0");
 *   Device.wifi.connect("wlan0", "MySSID", "password");
 *
 *   // VR
 *   const vr = Device.vr;
 *   if (vr.isAvailable()) { vr.initialize(); }
 *
 *   // Cast (Chromecast)
 *   const sinks = Device.aura.discover(3000);
 *   Device.aura.connect(sinks[0].ip, sinks[0].port);
 *   Device.aura.loadMedia("http://example.com/video.mp4");
 */
#include "modules/module_builders.h"

#include "content/device/device_manager.h"

#include <sstream>
#include <string>
#include <vector>

namespace modules::detail {
namespace {

// ──────────────────────────────────────────────────────────────────────────────
// Helpers

static v8::Local<v8::Object> DeviceInfoToObj(
    v8::Isolate* iso, v8::Local<v8::Context> ctx,
    const device::bluetooth::BluetoothDevice& d) {
    auto obj = v8::Object::New(iso);
    SetProperty(iso, ctx, obj, "address", Engine::Helper::ToV8Str(iso, d.address));
    SetProperty(iso, ctx, obj, "name",    Engine::Helper::ToV8Str(iso, d.name));
    SetProperty(iso, ctx, obj, "paired",   v8::Boolean::New(iso, d.is_paired));
    SetProperty(iso, ctx, obj, "connected",v8::Boolean::New(iso, d.is_connected));
    SetProperty(iso, ctx, obj, "rssi",     v8::Number::New(iso, d.rssi));
    return obj;
}

static v8::Local<v8::Object> UsbDeviceToObj(
    v8::Isolate* iso, v8::Local<v8::Context> ctx,
    const device::usb::UsbDevice& d) {
    auto obj = v8::Object::New(iso);
    SetProperty(iso, ctx, obj, "vendorId",  v8::Integer::New(iso, d.vendor_id));
    SetProperty(iso, ctx, obj, "productId", v8::Integer::New(iso, d.product_id));
    SetProperty(iso, ctx, obj, "manufacturer", Engine::Helper::ToV8Str(iso, d.manufacturer));
    SetProperty(iso, ctx, obj, "product",   Engine::Helper::ToV8Str(iso, d.product));
    SetProperty(iso, ctx, obj, "serial",    Engine::Helper::ToV8Str(iso, d.serial_number));
    SetProperty(iso, ctx, obj, "busdev",    Engine::Helper::ToV8Str(iso, d.busnum_devnum));
    return obj;
}

static v8::Local<v8::Object> WifiNetworkToObj(
    v8::Isolate* iso, v8::Local<v8::Context> ctx,
    const device::wifi::WifiNetwork& n) {
    auto obj = v8::Object::New(iso);
    SetProperty(iso, ctx, obj, "ssid",      Engine::Helper::ToV8Str(iso, n.ssid));
    SetProperty(iso, ctx, obj, "bssid",     Engine::Helper::ToV8Str(iso, n.bssid));
    SetProperty(iso, ctx, obj, "iface",     Engine::Helper::ToV8Str(iso, n.interface_name));
    SetProperty(iso, ctx, obj, "signal",    v8::Number::New(iso, n.signal_dbm));
    SetProperty(iso, ctx, obj, "frequency", v8::Number::New(iso, n.frequency_mhz));
    SetProperty(iso, ctx, obj, "connected", v8::Boolean::New(iso, n.is_connected));
    return obj;
}

// ──────────────────────────────────────────────────────────────────────────────
// Bluetooth callbacks

void BtScanCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    int duration_ms = 3000;
    if (args.Length() > 0 && args[0]->IsNumber())
        duration_ms = static_cast<int>(args[0]->NumberValue(ctx).FromMaybe(3000.0));

    std::vector<device::bluetooth::BluetoothDevice> found;
    device::DeviceManager::Instance().Bluetooth().StartScan(
        duration_ms,
        [&found](const device::bluetooth::BluetoothDevice& d) { found.push_back(d); });

    auto arr = v8::Array::New(iso, static_cast<int>(found.size()));
    for (size_t i = 0; i < found.size(); ++i)
        arr->Set(ctx, static_cast<uint32_t>(i), DeviceInfoToObj(iso, ctx, found[i])).Check();
    args.GetReturnValue().Set(arr);
}

void BtPairedCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    const auto paired = device::DeviceManager::Instance().Bluetooth().GetPairedDevices();
    auto arr = v8::Array::New(iso, static_cast<int>(paired.size()));
    for (size_t i = 0; i < paired.size(); ++i)
        arr->Set(ctx, static_cast<uint32_t>(i), DeviceInfoToObj(iso, ctx, paired[i])).Check();
    args.GetReturnValue().Set(arr);
}

void BtAvailableCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::Boolean::New(args.GetIsolate(),
                         device::DeviceManager::Instance().Bluetooth().IsAvailable()));
}

// ──────────────────────────────────────────────────────────────────────────────
// USB callbacks

void UsbEnumerateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    const auto devs = device::DeviceManager::Instance().Usb().EnumerateDevices();
    auto arr = v8::Array::New(iso, static_cast<int>(devs.size()));
    for (size_t i = 0; i < devs.size(); ++i)
        arr->Set(ctx, static_cast<uint32_t>(i), UsbDeviceToObj(iso, ctx, devs[i])).Check();
    args.GetReturnValue().Set(arr);
}

// ──────────────────────────────────────────────────────────────────────────────
// Wi-Fi callbacks

void WifiScanCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    const auto nets = device::DeviceManager::Instance().WifiScan().Scan();
    auto arr = v8::Array::New(iso, static_cast<int>(nets.size()));
    for (size_t i = 0; i < nets.size(); ++i)
        arr->Set(ctx, static_cast<uint32_t>(i), WifiNetworkToObj(iso, ctx, nets[i])).Check();
    args.GetReturnValue().Set(arr);
}

void WifiConnectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    if (args.Length() < 2) {
        args.GetReturnValue().Set(v8::Boolean::New(iso, false));
        return;
    }
    const std::string iface = args.Length() > 0 && args[0]->IsString()
        ? *v8::String::Utf8Value(iso, args[0]) : "wlan0";
    const std::string ssid = args.Length() > 1 && args[1]->IsString()
        ? *v8::String::Utf8Value(iso, args[1]) : "";
    const std::string psk  = args.Length() > 2 && args[2]->IsString()
        ? *v8::String::Utf8Value(iso, args[2]) : "";
    const bool ok = device::DeviceManager::Instance().Wifi().Connect(ssid, psk, iface);
    args.GetReturnValue().Set(v8::Boolean::New(iso, ok));
}

void WifiDisconnectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    const std::string iface = args.Length() > 0 && args[0]->IsString()
        ? *v8::String::Utf8Value(iso, args[0]) : "wlan0";
    const bool ok = device::DeviceManager::Instance().Wifi().Disconnect(iface);
    args.GetReturnValue().Set(v8::Boolean::New(iso, ok));
}

void WifiConnectivityCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const bool ok = device::DeviceManager::Instance().Wifi().CheckConnectivity();
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
}

// ──────────────────────────────────────────────────────────────────────────────
// VR callbacks

void VrAvailableCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const auto dev = device::DeviceManager::Instance().Vr().GetDevice();
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), dev.is_present));
}

void VrInitCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const bool ok = device::DeviceManager::Instance().Vr().Initialize();
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
}

void VrDeviceInfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    const auto dev = device::DeviceManager::Instance().Vr().GetDevice();
    auto obj = v8::Object::New(iso);
    SetProperty(iso, ctx, obj, "name",         Engine::Helper::ToV8Str(iso, dev.name));
    SetProperty(iso, ctx, obj, "manufacturer", Engine::Helper::ToV8Str(iso, dev.manufacturer));
    SetProperty(iso, ctx, obj, "renderWidth",  v8::Integer::NewFromUnsigned(iso, dev.render_width));
    SetProperty(iso, ctx, obj, "renderHeight", v8::Integer::NewFromUnsigned(iso, dev.render_height));
    SetProperty(iso, ctx, obj, "refreshRate",  v8::Number::New(iso, dev.refresh_rate));
    SetProperty(iso, ctx, obj, "isPresent",    v8::Boolean::New(iso, dev.is_present));
    args.GetReturnValue().Set(obj);
}

// ──────────────────────────────────────────────────────────────────────────────
// Aura Cast callbacks

void AuraDiscoverCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    int timeout_ms = 4000;
    if (args.Length() > 0 && args[0]->IsNumber())
        timeout_ms = static_cast<int>(args[0]->NumberValue(ctx).FromMaybe(4000.0));
    const auto sources =
        device::DeviceManager::Instance().AuraCast().ScanForSources(timeout_ms);
    auto arr = v8::Array::New(iso, static_cast<int>(sources.size()));
    for (size_t i = 0; i < sources.size(); ++i) {
        const auto& s = sources[i];
        auto obj = v8::Object::New(iso);
        SetProperty(iso, ctx, obj, "name",        Engine::Helper::ToV8Str(iso, s.broadcast_name));
        SetProperty(iso, ctx, obj, "addr",        Engine::Helper::ToV8Str(iso, s.addr_str));
        SetProperty(iso, ctx, obj, "rssi",        v8::Integer::New(iso, s.rssi));
        SetProperty(iso, ctx, obj, "broadcastId", v8::Integer::NewFromUnsigned(iso, s.broadcast_id));
        SetProperty(iso, ctx, obj, "encrypted",   v8::Boolean::New(iso, s.is_encrypted));
        SetProperty(iso, ctx, obj, "numBis",      v8::Integer::New(iso, s.num_bis));
        // Codec enum as string ("LC3" / "unknown").
        const char* codec_str =
            s.codec == device::cast::aura::AuraCodec::kLC3 ? "LC3" : "unknown";
        SetProperty(iso, ctx, obj, "codec",
                    Engine::Helper::ToV8Str(iso, codec_str));
        // Presentation delay in microseconds.
        SetProperty(iso, ctx, obj, "presentationDelayUs",
                    v8::Integer::NewFromUnsigned(iso, s.presentation_delay_us));
        // Array of BIS descriptors: [{bisIndex, channelCount, samplingFreqHz,
        //   frameDurationUs, octetsPerFrame}, ...].
        auto bis_arr = v8::Array::New(iso, static_cast<int>(s.bis_list.size()));
        for (size_t j = 0; j < s.bis_list.size(); ++j) {
            const auto& b = s.bis_list[j];
            auto bobj = v8::Object::New(iso);
            SetProperty(iso, ctx, bobj, "bisIndex",
                        v8::Integer::New(iso, b.bis_index));
            SetProperty(iso, ctx, bobj, "channelCount",
                        v8::Integer::New(iso, b.channel_count));
            SetProperty(iso, ctx, bobj, "samplingFreqHz",
                        v8::Integer::NewFromUnsigned(iso, b.sampling_freq_hz));
            SetProperty(iso, ctx, bobj, "frameDurationUs",
                        v8::Integer::NewFromUnsigned(iso, b.frame_duration_us));
            SetProperty(iso, ctx, bobj, "octetsPerFrame",
                        v8::Integer::New(iso, b.octets_per_frame));
            bis_arr->Set(ctx, static_cast<uint32_t>(j), bobj).Check();
        }
        SetProperty(iso, ctx, obj, "bisInfo", bis_arr);
        arr->Set(ctx, static_cast<uint32_t>(i), obj).Check();
    }
    args.GetReturnValue().Set(arr);
}

void AuraConnectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Auracast is BLE broadcast — no address parameter needed for scan result.
    // This is a no-op stub; callers should use SyncToSource via native API.
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
}

void AuraLoadMediaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Not applicable for Auracast (BLE LE Audio broadcast, receive-only).
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
}

// ──────────────────────────────────────────────────────────────────────────────
// DeviceManager-level callbacks

void DeviceInitCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    device::DeviceManager::Instance().Initialize();
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void DevicePerfReportCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const std::string report = device::DeviceManager::Instance().GetPerfReport();
    args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), report));
}

void DeviceMemStatsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* iso = args.GetIsolate();
    auto ctx  = iso->GetCurrentContext();
    const auto s = device::DeviceManager::Instance().GetMemoryStats();
    auto obj = v8::Object::New(iso);
    SetProperty(iso, ctx, obj, "totalMb",  v8::Number::New(iso, static_cast<double>(s.total_mb)));
    SetProperty(iso, ctx, obj, "usedMb",   v8::Number::New(iso, static_cast<double>(s.used_mb)));
    SetProperty(iso, ctx, obj, "freeMb",   v8::Number::New(iso, static_cast<double>(s.free_mb)));
    SetProperty(iso, ctx, obj, "usagePct", v8::Number::New(iso, s.usage_percent));
    args.GetReturnValue().Set(obj);
}

// ──────────────────────────────────────────────────────────────────────────────
// Sub-object builder helpers

static v8::Local<v8::Object> MakeBluetoothObj(v8::Isolate* iso, v8::Local<v8::Context> ctx) {
    auto obj = v8::Object::New(iso);
    Engine::Helper::SetMethod(iso, ctx, obj, "scan",      BtScanCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "paired",    BtPairedCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "available", BtAvailableCallback);
    return obj;
}

static v8::Local<v8::Object> MakeUsbObj(v8::Isolate* iso, v8::Local<v8::Context> ctx) {
    auto obj = v8::Object::New(iso);
    Engine::Helper::SetMethod(iso, ctx, obj, "enumerate", UsbEnumerateCallback);
    return obj;
}

static v8::Local<v8::Object> MakeWifiObj(v8::Isolate* iso, v8::Local<v8::Context> ctx) {
    auto obj = v8::Object::New(iso);
    Engine::Helper::SetMethod(iso, ctx, obj, "scan",         WifiScanCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "connect",      WifiConnectCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "disconnect",   WifiDisconnectCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "connectivity", WifiConnectivityCallback);
    return obj;
}

static v8::Local<v8::Object> MakeVrObj(v8::Isolate* iso, v8::Local<v8::Context> ctx) {
    auto obj = v8::Object::New(iso);
    Engine::Helper::SetMethod(iso, ctx, obj, "isAvailable", VrAvailableCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "initialize",  VrInitCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "deviceInfo",  VrDeviceInfoCallback);
    return obj;
}

static v8::Local<v8::Object> MakeAuraObj(v8::Isolate* iso, v8::Local<v8::Context> ctx) {
    auto obj = v8::Object::New(iso);
    Engine::Helper::SetMethod(iso, ctx, obj, "discover",   AuraDiscoverCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "connect",    AuraConnectCallback);
    Engine::Helper::SetMethod(iso, ctx, obj, "loadMedia",  AuraLoadMediaCallback);
    return obj;
}

}  // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Public entry point

bool BuildDeviceModule(v8::Isolate* isolate,
                       v8::Local<v8::Context> context,
                       v8::Local<v8::Object>* module_out,
                       std::string* error_out) {
    auto mod = v8::Object::New(isolate);

    Engine::Helper::SetMethod(isolate, context, mod, "initialize",   DeviceInitCallback);
    Engine::Helper::SetMethod(isolate, context, mod, "perfReport",   DevicePerfReportCallback);
    Engine::Helper::SetMethod(isolate, context, mod, "memStats",     DeviceMemStatsCallback);

    SetProperty(isolate, context, mod, "bluetooth", MakeBluetoothObj(isolate, context));
    SetProperty(isolate, context, mod, "usb",       MakeUsbObj(isolate, context));
    SetProperty(isolate, context, mod, "wifi",      MakeWifiObj(isolate, context));
    SetProperty(isolate, context, mod, "vr",        MakeVrObj(isolate, context));
    SetProperty(isolate, context, mod, "aura",      MakeAuraObj(isolate, context));

    *module_out = mod;
    return true;
}

}  // namespace modules::detail
