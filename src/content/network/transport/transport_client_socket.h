/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace network::transport {

using CompletionCallback = std::function<void(int /*bytes_or_error*/)>;

// Abstract client socket interface.
class TransportClientSocket {
public:
    virtual ~TransportClientSocket() = default;

    virtual bool Connect(const std::string& host,
                         uint16_t port,
                         std::string* error) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual int Read(uint8_t* buf, size_t len, std::string* error) = 0;
    virtual int Write(const uint8_t* buf, size_t len, std::string* error) = 0;

    virtual void ReadAsync(uint8_t* buf, size_t len, CompletionCallback cb) = 0;
    virtual void WriteAsync(const uint8_t* buf, size_t len, CompletionCallback cb) = 0;
};

}  // namespace network::transport
