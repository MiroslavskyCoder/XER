/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>

#include "content/network/proxy/proxy_config.h"

namespace network::proxy {

// SOCKS4/SOCKS5 client handshake helper.
class SocksClientSocket {
public:
    SocksClientSocket(int fd, const ProxyServer& proxy);

    // Perform the SOCKS handshake to connect to |target_host|:|target_port|.
    // Returns false on failure.
    bool Handshake(const std::string& target_host,
                   uint16_t target_port,
                   std::string* error);

private:
    bool Socks4Handshake(const std::string& host,
                         uint16_t port,
                         std::string* error);
    bool Socks5Handshake(const std::string& host,
                         uint16_t port,
                         std::string* error);

    int          fd_;
    ProxyServer  proxy_;
};

}  // namespace network::proxy
