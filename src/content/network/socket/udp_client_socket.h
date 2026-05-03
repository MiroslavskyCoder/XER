/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace network::socket {

class UdpClientSocket {
public:
    UdpClientSocket();
    ~UdpClientSocket();

    bool Bind(uint16_t local_port, std::string* error);
    bool Connect(const std::string& host, uint16_t port, std::string* error);
    void Close();
    bool IsOpen() const { return fd_ >= 0; }

    int SendTo(const uint8_t* buf, size_t len,
               const std::string& host, uint16_t port, std::string* error);
    int RecvFrom(std::vector<uint8_t>* buf,
                 std::string* from_host, uint16_t* from_port,
                 std::string* error);

private:
    int fd_ = -1;
};

}  // namespace network::socket
