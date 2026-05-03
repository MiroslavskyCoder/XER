/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <string>
#include <vector>

namespace network::proxy {

enum class ProxyType { kDirect, kHttp, kHttps, kSocks4, kSocks5 };

struct ProxyServer {
    ProxyType   type     = ProxyType::kDirect;
    std::string host;
    uint16_t    port     = 0;
    std::string username;
    std::string password;
};

struct ProxyConfig {
    bool        auto_detect   = false;
    std::string pac_url;        // PAC script URL
    ProxyServer proxy;          // manual proxy
    std::vector<std::string> bypass_list; // e.g. "localhost", "127.*"

    static ProxyConfig Direct();
    static ProxyConfig FromEnv();  // reads http_proxy / https_proxy env vars
};

}  // namespace network::proxy
