/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/socket/socket_server.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace network::socket {

SocketServer::~SocketServer() { Close(); }

bool SocketServer::Listen(const std::string& address, uint16_t port,
                          int backlog, std::string* error) {
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) { if (error) *error = std::strerror(errno); return false; }

    const int opt = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, address.empty() ? "0.0.0.0" : address.c_str(),
                &addr.sin_addr);

    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        ::listen(fd_, backlog) != 0) {
        if (error) *error = std::strerror(errno);
        Close(); return false;
    }
    return true;
}

std::unique_ptr<TcpClientSocket> SocketServer::Accept(std::string* error) {
    struct sockaddr_in client{};
    socklen_t len = sizeof(client);
    const int cfd = ::accept(fd_, reinterpret_cast<sockaddr*>(&client), &len);
    if (cfd < 0) { 
        if (error) *error = std::strerror(errno); 
        return nullptr; 
    }
    // Create a TcpClientSocket with the accepted file descriptor.
    // Uses private constructor via friend declaration.
    return std::make_unique<TcpClientSocket>(cfd);
}

void SocketServer::Close() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

}  // namespace network::socket
