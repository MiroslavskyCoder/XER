/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 *
 * Auracast is a Bluetooth® LE Audio broadcast standard defined by the
 * Bluetooth SIG (2022). It is built on:
 *   - Basic Audio Profile (BAP) 1.0.1
 *   - Public Broadcast Profile (PBP) 1.0
 *   - Common Audio Profile (CAP) 1.0
 *
 * Auracast transmitters announce available audio streams via BLE advertising
 * packets — specifically via the "Public Broadcast Announcement" service
 * (Service UUID 0x1856) in an AD type 0x16 (Service Data) record.
 * No pairing is required; any BLE 5.2+ device can receive.
 *
 * On Linux, scanning is done with a raw HCI socket (AF_BLUETOOTH / BTPROTO_HCI)
 * available in kernel 3.4+ through BlueZ 5.0+. Attaching to the actual audio
 * stream requires a BT_ISO (BTPROTO_ISO) socket added in kernel 5.15.
 *
 * Reference: https://en.wikipedia.org/wiki/Auracast
 *            https://en.wikipedia.org/wiki/Bluetooth_Low_Energy#Auracast
 */
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace device::cast::aura {

// Audio codec used by the Auracast source.  LC3 is the default LE Audio codec
// (ISO 21974 / Bluetooth SIG assigned number 0x06).
enum class AuraCodec : uint8_t {
    kLC3  = 0x06,  // Low Complexity Communication Codec — LE Audio default
    kSBC  = 0x01,  // Subband Coding (legacy, unlikely in Auracast)
    kUnknown = 0xFF,
};

// Public Broadcast Features byte (PBP spec, Table 3.3).
// Bit 0 → Encrypted, Bit 1 → Standard Quality, Bit 2 → High Quality.
struct PbpFeatures {
    bool encrypted        = false;
    bool standard_quality = false;
    bool high_quality     = false;

    explicit PbpFeatures(uint8_t raw) :
        encrypted       ((raw & 0x01) != 0),
        standard_quality((raw & 0x02) != 0),
        high_quality    ((raw & 0x04) != 0) {}
    PbpFeatures() = default;
};

// A single Broadcast Isochronous Stream (BIS) descriptor.
// A BIG can carry multiple BIS (e.g. left + right audio channel).
struct BisInfo {
    uint8_t bis_index = 0;    // 1-based index within the BIG
    uint8_t channel_count = 1;
    uint32_t sampling_freq_hz = 48000;  // Common LC3 rates: 8k 16k 24k 32k 44.1k 48k
    uint16_t frame_duration_us = 10000; // 7500 or 10000 µs (LC3)
    uint16_t octets_per_frame  = 120;
};

// An Auracast-compatible broadcast source discovered via BLE advertising scan.
// Discovered using BLE HCI passive scan + Public Broadcast Announcement
// AD type 0x16 with UUID 0x1856.
struct AuraCastDevice {
    // BLE identity
    std::array<uint8_t, 6> bd_addr = {};   // BD_ADDR (little-endian)
    uint8_t  bd_addr_type = 0;             // 0=Public, 1=Random
    std::string addr_str;                  // "AA:BB:CC:DD:EE:FF"
    int8_t   rssi = 0;                     // dBm; higher = closer

    // Auracast / PBP fields
    uint32_t broadcast_id = 0;            // 3-byte Broadcast_ID from advertising
    std::string broadcast_name;           // UTF-8 broadcast name (AD 0x30 or 0x09)
    PbpFeatures features;                  // PBP feature flags

    // BIG / BIS audio parameters
    uint8_t  big_handle = 0;              // BIG_Handle assigned at sync time
    uint8_t  num_bis = 0;                 // Number of BIS in the BIG
    std::vector<BisInfo> bis_list;

    AuraCodec codec = AuraCodec::kLC3;
    uint32_t presentation_delay_us = 0;   // Presentation delay (from BASE)

    // Runtime
    bool is_encrypted = false;
    bool is_available = true;
};

// Session state machine for a BLE Auracast sink (receiver).
enum class AuraCastState {
    kIdle,           // Not synced to any source
    kScanning,       // HCI BLE scan running
    kSyncing,        // PA (Periodic Advertising) sync in progress
    kSynced,         // Synced to PA, reading BASE
    kBigSyncing,     // BIG Create Sync in progress (BT_ISO connect)
    kStreaming,      // Audio BIS data flowing
    kError,
    kDisconnected,
};

}  // namespace device::cast::aura
