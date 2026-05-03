/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/socket/socket_client.h"
#include "content/network/network_util/network_util.h"

namespace network::socket {

std::unique_ptr<transport::TransportClientSocket>
CreateSocketForScheme(const std::string& scheme) {
    if (network::IsSecureScheme(scheme))
        return std::make_unique<SslClientSocket>();
    return std::make_unique<TcpClientSocket>();
}

}  // namespace network::socket
