/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "content/network/cert/scoped_nss_types.h"
#include "content/network/transport/transport_client_socket.h"

namespace network::socket {

class SslClientSocket : public transport::TransportClientSocket {
public:
    SslClientSocket();
    ~SslClientSocket() override;

    // Must be called before Connect.
    void SetCaBundlePath(const std::string& path) { ca_bundle_path_ = path; }
    void SetSkipVerify(bool skip) { skip_verify_ = skip; }

    bool Connect(const std::string& host,
                 uint16_t port,
                 std::string* error) override;
    void Disconnect() override;
    bool IsConnected() const override { return ssl_ != nullptr; }

    int Read(uint8_t* buf, size_t len, std::string* error) override;
    int Write(const uint8_t* buf, size_t len, std::string* error) override;

    void ReadAsync(uint8_t* buf, size_t len,
                   transport::CompletionCallback cb) override;
    void WriteAsync(const uint8_t* buf, size_t len,
                    transport::CompletionCallback cb) override;

private:
    std::string            ca_bundle_path_;
    bool                   skip_verify_ = false;
    int                    tcp_fd_      = -1;
    cert::ScopedSSLCtx     ctx_;
    cert::ScopedSSL        ssl_;
};

}  // namespace network::socket
