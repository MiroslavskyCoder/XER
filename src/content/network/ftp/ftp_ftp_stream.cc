/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/ftp/ftp_ftp_stream.h"

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>

namespace network::ftp {

FtpStream::~FtpStream() {
    if (ctrl_fd_ >= 0) ::close(ctrl_fd_);
}

bool FtpStream::Connect(const std::string& host, uint16_t port, std::string* error) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    const std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
        if (error) *error = "FTP: getaddrinfo failed";
        return false;
    }
    ctrl_fd_ = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    const bool ok = ctrl_fd_ >= 0 &&
                    ::connect(ctrl_fd_, res->ai_addr, res->ai_addrlen) == 0;
    freeaddrinfo(res);
    if (!ok) {
        if (error) *error = "FTP: connect failed";
        return false;
    }
    // Read welcome banner
    int code; std::string text, err;
    return ReadReply(&code, &text, error) && code == 220;
}

bool FtpStream::Login(const std::string& user, const std::string& pass, std::string* error) {
    int code; std::string text;
    if (!SendCommand("USER " + user, error)) return false;
    if (!ReadReply(&code, &text, error)) return false;
    if (code == 230) return true; // no password needed
    if (code != 331) { if (error) *error = "FTP: unexpected reply to USER"; return false; }
    if (!SendCommand("PASS " + pass, error)) return false;
    if (!ReadReply(&code, &text, error)) return false;
    if (code != 230) { if (error) *error = "FTP: login failed: " + text; return false; }
    return true;
}

bool FtpStream::SetBinaryMode(std::string* error) {
    int code; std::string text;
    return SendCommand("TYPE I", error) && ReadReply(&code, &text, error) && code == 200;
}

bool FtpStream::EnterPassive(std::string* data_host, uint16_t* data_port, std::string* error) {
    int code; std::string text;
    if (!SendCommand("PASV", error)) return false;
    if (!ReadReply(&code, &text, error) || code != 227) {
        if (error) *error = "FTP: PASV failed: " + text;
        return false;
    }
    // Parse (h1,h2,h3,h4,p1,p2)
    const size_t lp = text.find('('), rp = text.find(')');
    if (lp == std::string::npos || rp == std::string::npos) {
        if (error) *error = "FTP: bad PASV response";
        return false;
    }
    std::string nums = text.substr(lp+1, rp-lp-1);
    int h1,h2,h3,h4,p1,p2;
    if (std::sscanf(nums.c_str(), "%d,%d,%d,%d,%d,%d",
                    &h1,&h2,&h3,&h4,&p1,&p2) != 6) {
        if (error) *error = "FTP: PASV parse error";
        return false;
    }
    *data_host = std::to_string(h1)+"."+std::to_string(h2)+"."+
                 std::to_string(h3)+"."+std::to_string(h4);
    *data_port  = static_cast<uint16_t>(p1 * 256 + p2);
    return true;
}

bool FtpStream::Retrieve(const std::string& path,
                          std::vector<uint8_t>* out,
                          std::string* error) {
    std::string dhost; uint16_t dport = 0;
    if (!SetBinaryMode(error) || !EnterPassive(&dhost, &dport, error)) return false;

    // Open data connection
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    const std::string ps = std::to_string(dport);
    if (getaddrinfo(dhost.c_str(), ps.c_str(), &hints, &res) != 0 || !res) {
        if (error) *error = "FTP: data connect failed";
        return false;
    }
    const int dfd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bool ok = dfd >= 0 && ::connect(dfd, res->ai_addr, res->ai_addrlen) == 0;
    freeaddrinfo(res);
    if (!ok) { if (error) *error = "FTP: data connection refused"; return false; }

    if (!SendCommand("RETR " + path, error)) { ::close(dfd); return false; }
    int code; std::string text;
    if (!ReadReply(&code, &text, error) || (code != 125 && code != 150)) {
        ::close(dfd); if (error) *error = "FTP: RETR rejected"; return false;
    }
    uint8_t buf[8192];
    ssize_t n;
    while ((n = ::read(dfd, buf, sizeof(buf))) > 0)
        out->insert(out->end(), buf, buf + n);
    ::close(dfd);
    ReadReply(&code, &text, error); // 226 Transfer complete
    return true;
}

bool FtpStream::List(const std::string& path,
                      std::vector<std::string>* out,
                      std::string* error) {
    std::string dhost; uint16_t dport = 0;
    if (!EnterPassive(&dhost, &dport, error)) return false;

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    const std::string ps = std::to_string(dport);
    if (getaddrinfo(dhost.c_str(), ps.c_str(), &hints, &res) != 0 || !res) {
        if (error) *error = "FTP: list data connect failed";
        return false;
    }
    const int dfd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bool ok = dfd >= 0 && ::connect(dfd, res->ai_addr, res->ai_addrlen) == 0;
    freeaddrinfo(res);
    if (!ok) return false;

    if (!SendCommand("LIST " + path, error)) { ::close(dfd); return false; }
    int code; std::string text;
    ReadReply(&code, &text, error);

    std::string raw;
    char buf[4096];
    ssize_t n;
    while ((n = ::read(dfd, buf, sizeof(buf))) > 0)
        raw.append(buf, n);
    ::close(dfd);
    ReadReply(&code, &text, error);

    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) out->push_back(line);
    }
    return true;
}

bool FtpStream::Quit() {
    std::string err;
    SendCommand("QUIT", &err);
    if (ctrl_fd_ >= 0) { ::close(ctrl_fd_); ctrl_fd_ = -1; }
    return true;
}

bool FtpStream::SendCommand(const std::string& cmd, std::string* error) {
    const std::string line = cmd + "\r\n";
    const ssize_t n = ::write(ctrl_fd_, line.data(), line.size());
    if (n < 0) { if (error) *error = "FTP: write error"; return false; }
    return true;
}

bool FtpStream::ReadReply(int* code, std::string* text, std::string* error) {
    std::string line;
    char c;
    while (::read(ctrl_fd_, &c, 1) == 1) {
        if (c == '\n') break;
        if (c != '\r') line.push_back(c);
    }
    if (line.size() < 3) { if (error) *error = "FTP: short reply"; return false; }
    try { *code = std::stoi(line.substr(0, 3)); }
    catch (...) { if (error) *error = "FTP: bad code"; return false; }
    if (text) *text = line.substr(3);
    return true;
}

FtpResponse ExecuteFtpRequest(const FtpRequest& req) {
    FtpResponse resp;
    FtpStream stream;
    if (!stream.Connect(req.host, req.port, &resp.error)) return resp;
    if (!stream.Login(req.username, req.password, &resp.error)) return resp;
    if (!stream.Retrieve(req.remote_path, &resp.data, &resp.error)) return resp;
    stream.Quit();
    resp.ok = true;
    return resp;
}

}  // namespace network::ftp
