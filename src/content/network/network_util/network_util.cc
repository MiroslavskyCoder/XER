/**
 * This file is part of XER, Network open source project.
 *
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/network_util/network_util.h"

#include <algorithm>
#include <regex>
#include <sstream>

namespace network {

std::string NetErrorToString(NetError error) {
    switch (error) {
        case NetError::kOk:                    return "OK";
        case NetError::kPending:               return "ERR_IO_PENDING";
        case NetError::kFailed:                return "ERR_FAILED";
        case NetError::kAborted:               return "ERR_ABORTED";
        case NetError::kInvalidArgument:       return "ERR_INVALID_ARGUMENT";
        case NetError::kTimedOut:              return "ERR_TIMED_OUT";
        case NetError::kConnectionClosed:      return "ERR_CONNECTION_CLOSED";
        case NetError::kConnectionReset:       return "ERR_CONNECTION_RESET";
        case NetError::kConnectionRefused:     return "ERR_CONNECTION_REFUSED";
        case NetError::kConnectionAborted:     return "ERR_CONNECTION_ABORTED";
        case NetError::kConnectionFailed:      return "ERR_CONNECTION_FAILED";
        case NetError::kNameNotResolved:       return "ERR_NAME_NOT_RESOLVED";
        case NetError::kAddressUnreachable:    return "ERR_ADDRESS_UNREACHABLE";
        case NetError::kSslProtocolError:      return "ERR_SSL_PROTOCOL_ERROR";
        case NetError::kCertCommonNameInvalid: return "ERR_CERT_COMMON_NAME_INVALID";
        case NetError::kCertDateInvalid:       return "ERR_CERT_DATE_INVALID";
        case NetError::kCertAuthorityInvalid:  return "ERR_CERT_AUTHORITY_INVALID";
        case NetError::kCertRevoked:           return "ERR_CERT_REVOKED";
        case NetError::kCertInvalid:           return "ERR_CERT_INVALID";
        case NetError::kProxyConnectionFailed: return "ERR_PROXY_CONNECTION_FAILED";
        case NetError::kSocksConnectionFailed: return "ERR_SOCKS_CONNECTION_FAILED";
        case NetError::kDnsMalformedResponse:  return "ERR_DNS_MALFORMED_RESPONSE";
        default:                               return "ERR_UNKNOWN";
    }
}

bool IsValidPort(int port) {
    return port >= 1 && port <= 65535;
}

bool IsIPv6Literal(const std::string& host) {
    return host.size() >= 2 && host.front() == '[' && host.back() == ']';
}

bool IsIPLiteral(const std::string& host) {
    if (IsIPv6Literal(host)) return true;
    static const std::regex kIPv4(R"(^(\d{1,3}\.){3}\d{1,3}$)");
    return std::regex_match(host, kIPv4);
}

std::string UnwrapIPv6(const std::string& host) {
    if (IsIPv6Literal(host))
        return host.substr(1, host.size() - 2);
    return host;
}

std::string CanonicaliseHost(const std::string& host) {
    std::string out = host;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    // Strip trailing dot.
    if (!out.empty() && out.back() == '.')
        out.pop_back();
    return out;
}

std::string MakeHostPortPair(const std::string& host, std::uint16_t port) {
    std::ostringstream oss;
    if (host.find(':') != std::string::npos && !IsIPv6Literal(host))
        oss << '[' << host << ']';
    else
        oss << host;
    oss << ':' << port;
    return oss.str();
}

}  // namespace network
