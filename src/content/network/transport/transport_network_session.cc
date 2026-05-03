/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/transport/transport_network_session.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace network::transport {

TransportNetworkSession::TransportNetworkSession(
    TransportSessionParams params,
    TransportNetworkDelegate* delegate)
    : params_(std::move(params)), delegate_(delegate) {}

TransportNetworkSession::~TransportNetworkSession() {
    Close();
}

bool TransportNetworkSession::Open(std::string* error) {
    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    const std::string port_str = std::to_string(params_.port);
    const int rc = ::getaddrinfo(params_.host.c_str(), port_str.c_str(), &hints, &res);
    if (rc != 0) {
        if (error) *error = ::gai_strerror(rc);
        return false;
    }

    int fd = -1;
    for (auto* p = res; p; p = p->ai_next) {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
        ::close(fd);
        fd = -1;
    }
    ::freeaddrinfo(res);

    if (fd < 0) {
        if (error) *error = "connect failed: " + std::string(std::strerror(errno));
        return false;
    }
    fd_ = fd;
    if (delegate_) delegate_->OnConnected(params_.host, params_.port);
    return true;
}

void TransportNetworkSession::Close() {
    if (fd_ >= 0) {
        ::close(fd_);
        if (delegate_) delegate_->OnDisconnected(params_.host, params_.port);
        fd_ = -1;
    }
}

bool TransportNetworkSession::IsOpen() const { return fd_ >= 0; }

int TransportNetworkSession::Read(uint8_t* buf, size_t len, std::string* error) {
    const ssize_t n = ::recv(fd_, buf, len, 0);
    if (n < 0) {
        if (error) *error = std::strerror(errno);
        return -1;
    }
    if (delegate_) delegate_->OnBytesReceived(static_cast<size_t>(n));
    return static_cast<int>(n);
}

int TransportNetworkSession::Write(const uint8_t* buf, size_t len, std::string* error) {
    const ssize_t n = ::send(fd_, buf, len, MSG_NOSIGNAL);
    if (n < 0) {
        if (error) *error = std::strerror(errno);
        return -1;
    }
    if (delegate_) delegate_->OnBytesSent(static_cast<size_t>(n));
    return static_cast<int>(n);
}

}  // namespace network::transport
