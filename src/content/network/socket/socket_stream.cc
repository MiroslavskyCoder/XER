/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/socket/socket_stream.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "content/network/socket/net_log.h"

namespace network::socket {

SocketStream::SocketStream(int fd) : fd_(fd) {}
SocketStream::~SocketStream() { if (fd_ >= 0) ::close(fd_); }

bool SocketStream::ReadExact(std::vector<uint8_t>* buf,
                              size_t n,
                              std::string* error) {
    buf->resize(n);
    size_t got = 0;
    while (got < n) {
        const ssize_t r = ::recv(fd_, buf->data() + got, n - got, MSG_WAITALL);
        if (r <= 0) {
            if (error) *error = r == 0 ? "connection closed" : std::strerror(errno);
            return false;
        }
        got += static_cast<size_t>(r);
    }
    return true;
}

bool SocketStream::ReadLine(std::string* line, std::string* error) {
    line->clear();
    char c;
    while (true) {
        const ssize_t r = ::recv(fd_, &c, 1, 0);
        if (r == 0) { if (error) *error = "connection closed"; return false; }
        if (r < 0)  { if (error) *error = std::strerror(errno); return false; }
        line->push_back(c);
        if (line->size() >= 2 &&
            (*line)[line->size()-2] == '\r' &&
            (*line)[line->size()-1] == '\n') break;
    }
    return true;
}

bool SocketStream::WriteAll(const uint8_t* data, size_t len, std::string* error) {
    size_t sent = 0;
    while (sent < len) {
        const ssize_t r = ::send(fd_, data + sent, len - sent, MSG_NOSIGNAL);
        if (r <= 0) {
            if (error) *error = std::strerror(errno);
            return false;
        }
        sent += static_cast<size_t>(r);
    }
    return true;
}

bool SocketStream::WriteAll(const std::string& data, std::string* error) {
    return WriteAll(reinterpret_cast<const uint8_t*>(data.data()),
                    data.size(), error);
}

}  // namespace network::socket
