/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/proxy/socks_client_socket.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <vector>

namespace network::proxy {

SocksClientSocket::SocksClientSocket(int fd, const ProxyServer& proxy)
    : fd_(fd), proxy_(proxy) {}

bool SocksClientSocket::Handshake(const std::string& host,
                                  uint16_t port,
                                  std::string* error) {
    if (proxy_.type == ProxyType::kSocks4)
        return Socks4Handshake(host, port, error);
    return Socks5Handshake(host, port, error);
}

bool SocksClientSocket::Socks4Handshake(const std::string& host,
                                         uint16_t port,
                                         std::string* error) {
    struct in_addr addr{};
    if (::inet_pton(AF_INET, host.c_str(), &addr) != 1) {
        if (error) *error = "SOCKS4 requires IPv4 literal";
        return false;
    }
    std::vector<uint8_t> req = {
        0x04, 0x01,
        static_cast<uint8_t>(port >> 8),
        static_cast<uint8_t>(port & 0xff),
        static_cast<uint8_t>((ntohl(addr.s_addr) >> 24) & 0xff),
        static_cast<uint8_t>((ntohl(addr.s_addr) >> 16) & 0xff),
        static_cast<uint8_t>((ntohl(addr.s_addr) >>  8) & 0xff),
        static_cast<uint8_t>( ntohl(addr.s_addr)        & 0xff),
        0x00  // user ID (null)
    };
    ::send(fd_, req.data(), req.size(), MSG_NOSIGNAL);
    uint8_t resp[8]{};
    if (::recv(fd_, resp, 8, MSG_WAITALL) != 8 || resp[1] != 0x5a) {
        if (error) *error = "SOCKS4 handshake failed";
        return false;
    }
    return true;
}

bool SocksClientSocket::Socks5Handshake(const std::string& host,
                                         uint16_t port,
                                         std::string* error) {
    // Auth negotiation
    const bool has_auth = !proxy_.username.empty();
    const uint8_t auth_method = has_auth ? 0x02 : 0x00;
    uint8_t greet[] = {0x05, 0x01, auth_method};
    ::send(fd_, greet, 3, MSG_NOSIGNAL);
    uint8_t resp[2]{};
    if (::recv(fd_, resp, 2, MSG_WAITALL) != 2 || resp[0] != 0x05) {
        if (error) *error = "SOCKS5 greeting failed";
        return false;
    }
    if (resp[1] == 0x02 && has_auth) {
        // Username/password auth (RFC 1929)
        std::vector<uint8_t> auth;
        auth.push_back(0x01);
        auth.push_back(static_cast<uint8_t>(proxy_.username.size()));
        for (char c : proxy_.username) auth.push_back(static_cast<uint8_t>(c));
        auth.push_back(static_cast<uint8_t>(proxy_.password.size()));
        for (char c : proxy_.password) auth.push_back(static_cast<uint8_t>(c));
        ::send(fd_, auth.data(), auth.size(), MSG_NOSIGNAL);
        uint8_t ar[2]{};
        if (::recv(fd_, ar, 2, MSG_WAITALL) != 2 || ar[1] != 0x00) {
            if (error) *error = "SOCKS5 auth rejected";
            return false;
        }
    }
    // Connect request
    std::vector<uint8_t> req = {0x05, 0x01, 0x00, 0x03};
    req.push_back(static_cast<uint8_t>(host.size()));
    for (char c : host) req.push_back(static_cast<uint8_t>(c));
    req.push_back(static_cast<uint8_t>(port >> 8));
    req.push_back(static_cast<uint8_t>(port & 0xff));
    ::send(fd_, req.data(), req.size(), MSG_NOSIGNAL);

    uint8_t conn[10]{};
    if (::recv(fd_, conn, 10, 0) < 4 || conn[1] != 0x00) {
        if (error) *error = "SOCKS5 connect failed";
        return false;
    }
    return true;
}

}  // namespace network::proxy
