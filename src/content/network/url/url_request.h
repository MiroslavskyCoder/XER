/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace network::url {

struct URLResponse {
    int         status_code = 0;
    std::string status_text;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
};

using URLResponseCallback = std::function<void(bool ok,
                                               const URLResponse& response,
                                               const std::string& error)>;

struct URLRequest {
    std::string method  = "GET";
    std::string url;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
    uint32_t timeout_ms = 30000;
    bool     follow_redirects = true;
    int      max_redirects    = 5;
};

}  // namespace network::url
