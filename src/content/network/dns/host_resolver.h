/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace network::dns {

struct ResolvedAddress {
    std::string address;   // IPv4 or IPv6 string
    uint32_t    ttl_sec = 0;
};

using ResolveCallback = std::function<void(bool ok,
                                           const std::vector<ResolvedAddress>& addrs,
                                           const std::string& error)>;

// Abstract interface: resolves hostnames to IP addresses.
class HostResolver {
public:
    virtual ~HostResolver() = default;

    // Synchronously resolve |host|. Returns false on failure.
    virtual bool Resolve(const std::string& host,
                         uint16_t port,
                         std::vector<ResolvedAddress>* out,
                         std::string* error) = 0;

    // Asynchronously resolve |host|. Callback is called on completion.
    virtual void ResolveAsync(const std::string& host,
                              uint16_t port,
                              ResolveCallback callback) = 0;
};

}  // namespace network::dns
