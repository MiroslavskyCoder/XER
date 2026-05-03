/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/socket/udp_client_socket.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace network::socket {

UdpClientSocket::UdpClientSocket() {}
UdpClientSocket::~UdpClientSocket() { Close(); }

bool UdpClientSocket::Bind(uint16_t local_port, std::string* error) {
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) { if (error) *error = std::strerror(errno); return false; }
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(local_port);
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        if (error) *error = std::strerror(errno);
        Close(); return false;
    }
    return true;
}

bool UdpClientSocket::Connect(const std::string& host, uint16_t port,
                               std::string* error) {
    if (fd_ < 0) {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) { if (error) *error = std::strerror(errno); return false; }
    }
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        if (error) *error = std::strerror(errno);
        return false;
    }
    return true;
}

void UdpClientSocket::Close() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

int UdpClientSocket::SendTo(const uint8_t* buf, size_t len,
                             const std::string& host, uint16_t port,
                             std::string* error) {
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    const ssize_t n = ::sendto(fd_, buf, len, 0,
                               reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (n < 0 && error) *error = std::strerror(errno);
    return static_cast<int>(n);
}

int UdpClientSocket::RecvFrom(std::vector<uint8_t>* buf,
                               std::string* from_host, uint16_t* from_port,
                               std::string* error) {
    buf->resize(65536);
    struct sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    const ssize_t n = ::recvfrom(fd_, buf->data(), buf->size(), 0,
                                 reinterpret_cast<sockaddr*>(&from), &from_len);
    if (n < 0) { if (error) *error = std::strerror(errno); return -1; }
    buf->resize(static_cast<size_t>(n));
    char host_buf[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &from.sin_addr, host_buf, sizeof(host_buf));
    if (from_host) *from_host = host_buf;
    if (from_port) *from_port = ntohs(from.sin_port);
    return static_cast<int>(n);
}

}  // namespace network::socket
