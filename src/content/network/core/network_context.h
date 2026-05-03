/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <memory>
#include <string>

#include "content/network/url/url_request_context.h"
#include "content/network/http/http_network_session.h"
#include "content/network/http/http_cache.h"
#include "content/network/http/http_server_properties.h"
#include "content/network/dns/dns_config_service.h"
#include "content/network/cert/cert_storage.h"
#include "content/network/proxy/proxy_config.h"

namespace network::core {

// Top-level context aggregating all network subsystem singletons.
class NetworkContext {
public:
    static NetworkContext& Instance();

    url::URLRequestContext&          RequestContext()     { return request_ctx_; }
    http::HttpNetworkSession&        HttpSession()        { return http::HttpNetworkSession::Default(); }
    http::HttpCache&                 Cache()              { return http::HttpCache::Instance(); }
    http::HttpServerProperties&      ServerProperties()   { return http::HttpServerProperties::Instance(); }

    void Reset();

private:
    NetworkContext() = default;
    url::URLRequestContext request_ctx_;
};

}  // namespace network::core
