/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/socket/ssl_client_socket.h"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "async_io/io_thread_pool.h"
#include "content/network/cert/cert_storage.h"
#include "content/network/socket/net_log.h"

namespace network::socket {

SslClientSocket::SslClientSocket() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SslClientSocket::~SslClientSocket() { Disconnect(); }

bool SslClientSocket::Connect(const std::string& host,
                               uint16_t port,
                               std::string* error) {
    // TCP connect
    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = nullptr;
    const std::string port_str = std::to_string(port);
    if (::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
        if (error) *error = "getaddrinfo failed";
        return false;
    }
    for (auto* p = res; p; p = p->ai_next) {
        tcp_fd_ = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (tcp_fd_ < 0) continue;
        if (::connect(tcp_fd_, p->ai_addr, p->ai_addrlen) == 0) break;
        ::close(tcp_fd_); tcp_fd_ = -1;
    }
    ::freeaddrinfo(res);
    if (tcp_fd_ < 0) { if (error) *error = "TCP connect failed"; return false; }

    // TLS setup
    SSL_CTX* raw_ctx = SSL_CTX_new(TLS_client_method());
    if (!raw_ctx) { if (error) *error = "SSL_CTX_new failed"; return false; }
    ctx_.reset(raw_ctx);

    if (!skip_verify_) {
        const std::string bundle = ca_bundle_path_.empty()
            ? cert::CertStorage::SystemCaBundlePath()
            : ca_bundle_path_;
        if (!bundle.empty())
            SSL_CTX_load_verify_locations(ctx_.get(), bundle.c_str(), nullptr);
        SSL_CTX_set_verify(ctx_.get(), SSL_VERIFY_PEER, nullptr);
    }

    SSL* raw_ssl = SSL_new(ctx_.get());
    if (!raw_ssl) { if (error) *error = "SSL_new failed"; return false; }
    ssl_.reset(raw_ssl);

    SSL_set_fd(ssl_.get(), tcp_fd_);
    SSL_set_tlsext_host_name(ssl_.get(), host.c_str());

    if (SSL_connect(ssl_.get()) != 1) {
        unsigned long err_code = ERR_get_error();
        char buf[256]{};
        ERR_error_string_n(err_code, buf, sizeof(buf));
        if (error) *error = buf;
        ssl_.reset(); ctx_.reset();
        ::close(tcp_fd_); tcp_fd_ = -1;
        return false;
    }
    NetLog::Info("TLS connected to " + host + ":" + std::to_string(port));
    return true;
}

void SslClientSocket::Disconnect() {
    if (ssl_) { SSL_shutdown(ssl_.get()); ssl_.reset(); }
    if (ctx_) ctx_.reset();
    if (tcp_fd_ >= 0) { ::close(tcp_fd_); tcp_fd_ = -1; }
}

int SslClientSocket::Read(uint8_t* buf, size_t len, std::string* error) {
    if (!ssl_) { if (error) *error = "not connected"; return -1; }
    const int n = SSL_read(ssl_.get(), buf, static_cast<int>(len));
    if (n <= 0 && error) {
        char buf2[256]{};
        ERR_error_string_n(ERR_get_error(), buf2, sizeof(buf2));
        *error = buf2;
    }
    return n;
}

int SslClientSocket::Write(const uint8_t* buf, size_t len, std::string* error) {
    if (!ssl_) { if (error) *error = "not connected"; return -1; }
    const int n = SSL_write(ssl_.get(), buf, static_cast<int>(len));
    if (n <= 0 && error) {
        char buf2[256]{};
        ERR_error_string_n(ERR_get_error(), buf2, sizeof(buf2));
        *error = buf2;
    }
    return n;
}

void SslClientSocket::ReadAsync(uint8_t* buf, size_t len,
                                 transport::CompletionCallback cb) {
    IO::AsyncIO::IOThreadPool::GetSharedInstance().Enqueue(
        [this, buf, len, cb = std::move(cb)]() {
            std::string err;
            cb(Read(buf, len, &err));
        });
}

void SslClientSocket::WriteAsync(const uint8_t* buf, size_t len,
                                  transport::CompletionCallback cb) {
    std::vector<uint8_t> data(buf, buf + len);
    IO::AsyncIO::IOThreadPool::GetSharedInstance().Enqueue(
        [this, data = std::move(data), cb = std::move(cb)]() mutable {
            std::string err;
            cb(Write(data.data(), data.size(), &err));
        });
}

}  // namespace network::socket
