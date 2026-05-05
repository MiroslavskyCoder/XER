/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/device/wifi/wifi_connection_manager.h"
#include "content/device/wifi/wifi_scanner.h"

#include <fstream>
#include <sstream>
#include <cstdio>

// Project network stack — DNS resolver for connectivity check.
#include "content/network/dns/host_resolver_impl.h"
// Project async thread pool — background scans.
#include "async_io/io_thread_pool.h"

#if defined(__linux__)
#include <unistd.h>
#endif

namespace device::wifi {

namespace {

// Run |cmd| via popen, return stdout as string.
static std::string RunWpaCli(const std::string& args) {
    const std::string cmd = "wpa_cli " + args + " 2>/dev/null";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return {};
    std::string out;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) out += buf;
    pclose(fp);
    return out;
}

// Parse /proc/net/dev line for |iface|.
static WifiConnectionManager::NetStats ReadNetStats(const std::string& iface) {
    WifiConnectionManager::NetStats s{};
    std::ifstream f("/proc/net/dev");
    if (!f.good()) return s;
    std::string line;
    std::getline(f, line); std::getline(f, line); // skip headers
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string name;
        ss >> name;
        if (name.empty()) continue;
        if (name.back() == ':') name.pop_back();
        if (name != iface) continue;
        ss >> s.rx_bytes >> s.rx_packets >> s.rx_errors;
        // skip: drop fifo frame compressed multicast
        uint64_t skip;
        for (int i = 0; i < 5; ++i) ss >> skip;
        ss >> s.tx_bytes >> s.tx_packets >> s.tx_errors;
        break;
    }
    return s;
}

}  // namespace

// ──────────────────────────────────────────────────────────────────────────────

WifiConnectionManager& WifiConnectionManager::Instance() {
    static WifiConnectionManager inst;
    return inst;
}

bool WifiConnectionManager::Connect(const std::string& ssid,
                                     const std::string& passphrase,
                                     const std::string& interface,
                                     ConnectionStateCallback callback) {
    if (callback) callback(WifiConnectionState::kAuthenticating, {});

    // Add network via wpa_cli.
    const std::string netid_str = RunWpaCli("-i " + interface + " add_network");
    if (netid_str.empty() || netid_str[0] < '0' || netid_str[0] > '9') {
        if (callback) callback(WifiConnectionState::kFailed, "add_network failed");
        return false;
    }
    const std::string nid(1, netid_str[0]);
    RunWpaCli("-i " + interface + " set_network " + nid + " ssid '\"" + ssid + "\"'");
    RunWpaCli("-i " + interface + " set_network " + nid + " psk '\"" + passphrase + "\"'");
    RunWpaCli("-i " + interface + " enable_network " + nid);

    if (callback) callback(WifiConnectionState::kAssociating, {});

    const std::string status = RunWpaCli("-i " + interface + " select_network " + nid);
    const bool ok = (status.find("OK") != std::string::npos);
    if (!ok) {
        if (callback) callback(WifiConnectionState::kFailed, "select_network failed");
        return false;
    }
    if (callback) callback(WifiConnectionState::kConnected, {});
    return true;
}

bool WifiConnectionManager::Disconnect(const std::string& interface) {
    const std::string out = RunWpaCli("-i " + interface + " disconnect");
    return out.find("OK") != std::string::npos;
}

WifiConnectionState WifiConnectionManager::GetState(
    const std::string& interface) const {
    const std::string out = RunWpaCli("-i " + interface + " status");
    if (out.find("wpa_state=COMPLETED") != std::string::npos)
        return WifiConnectionState::kConnected;
    if (out.find("wpa_state=ASSOCIATING") != std::string::npos)
        return WifiConnectionState::kAssociating;
    if (out.find("wpa_state=4WAY_HANDSHAKE") != std::string::npos ||
        out.find("wpa_state=GROUP_HANDSHAKE") != std::string::npos)
        return WifiConnectionState::kAuthenticating;
    if (out.find("wpa_state=DISCONNECTED") != std::string::npos ||
        out.find("wpa_state=INACTIVE") != std::string::npos)
        return WifiConnectionState::kDisconnected;
    return WifiConnectionState::kDisconnected;
}

void WifiConnectionManager::ScanAsync(
    const std::string& interface,
    std::function<void(std::vector<WifiNetwork>)> cb) {
    // Run scan on the shared IO thread pool so the caller is not blocked.
    IO::AsyncIO::IOThreadPool::GetSharedInstance().Enqueue(
        [interface, cb = std::move(cb)]() mutable {
            RunWpaCli("-i " + interface + " scan");
            // Small wait for kernel scan results to arrive.
#if defined(__linux__)
            usleep(3'000'000);  // 3 s
#endif
            auto results = WifiScanner::Instance().Scan();
            if (cb) cb(std::move(results));
        });
}

bool WifiConnectionManager::CheckConnectivity(const std::string& host) const {
    // Use the project DNS resolver to verify we can reach the internet.
    network::dns::HostResolverImpl resolver;
    std::vector<network::dns::ResolvedAddress> addrs;
    std::string err;
    return resolver.Resolve(host, 80, &addrs, &err) && !addrs.empty();
}

WifiConnectionManager::NetStats
WifiConnectionManager::GetNetStats(const std::string& interface) const {
    return ReadNetStats(interface);
}

}  // namespace device::wifi
