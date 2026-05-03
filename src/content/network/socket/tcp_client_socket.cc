/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/socket/tcp_client_socket.h"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "content/network/socket/net_log.h"
#include "async_io/io_thread_pool.h"

namespace network::socket {

TcpClientSocket::~TcpClientSocket() { Disconnect(); }

bool TcpClientSocket::Connect(const std::string& host,
                               uint16_t port,
                               std::string* error) {
    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    const std::string port_str = std::to_string(port);
    const int rc = ::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (rc != 0) {
        if (error) *error = ::gai_strerror(rc);
        return false;
    }

    for (auto* p = res; p; p = p->ai_next) {
        fd_ = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd_ < 0) continue;
        if (::connect(fd_, p->ai_addr, p->ai_addrlen) == 0) break;
        ::close(fd_); fd_ = -1;
    }
    ::freeaddrinfo(res);

    if (fd_ < 0) {
        if (error) *error = std::strerror(errno);
        return false;
    }
    stream_ = std::make_unique<SocketStream>(fd_);
    NetLog::Info("TCP connected to " + host + ":" + std::to_string(port));
    return true;
}

void TcpClientSocket::Disconnect() {
    stream_.reset();
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

bool TcpClientSocket::IsConnected() const { return fd_ >= 0; }

int TcpClientSocket::Read(uint8_t* buf, size_t len, std::string* error) {
    const ssize_t n = ::recv(fd_, buf, len, 0);
    if (n < 0 && error) *error = std::strerror(errno);
    return static_cast<int>(n);
}

int TcpClientSocket::Write(const uint8_t* buf, size_t len, std::string* error) {
    const ssize_t n = ::send(fd_, buf, len, MSG_NOSIGNAL);
    if (n < 0 && error) *error = std::strerror(errno);
    return static_cast<int>(n);
}

void TcpClientSocket::ReadAsync(uint8_t* buf, size_t len,
                                 transport::CompletionCallback cb) {
    IO::AsyncIO::IOThreadPool::GetSharedInstance().Enqueue(
        [this, buf, len, cb = std::move(cb)]() {
            std::string err;
            const int n = Read(buf, len, &err);
            cb(n);
        });
}

void TcpClientSocket::WriteAsync(const uint8_t* buf, size_t len,
                                  transport::CompletionCallback cb) {
    // Copy data so we can enqueue safely
    std::vector<uint8_t> data(buf, buf + len);
    IO::AsyncIO::IOThreadPool::GetSharedInstance().Enqueue(
        [this, data = std::move(data), cb = std::move(cb)]() mutable {
            std::string err;
            const int n = Write(data.data(), data.size(), &err);
            cb(n);
        });
}

}  // namespace network::socket
