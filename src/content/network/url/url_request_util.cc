/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/url/url_request_util.h"
#include "content/network/url/url_request_job.h"

namespace network::url {

URLResponse SyncGet(const std::string& url,
                    const URLRequestContext& ctx,
                    std::string* error) {
    URLRequest req;
    req.method = "GET";
    req.url    = url;
    URLResponse resp;
    std::string local_err;
    URLRequestJob job(ctx);
    job.Run(req, &resp, error ? error : &local_err);
    return resp;
}

URLResponse SyncPost(const std::string& url,
                     const std::string& body,
                     const std::string& content_type,
                     const URLRequestContext& ctx,
                     std::string* error) {
    URLRequest req;
    req.method = "POST";
    req.url    = url;
    req.headers["Content-Type"] = content_type;
    req.body.assign(body.begin(), body.end());
    URLResponse resp;
    std::string local_err;
    URLRequestJob job(ctx);
    job.Run(req, &resp, error ? error : &local_err);
    return resp;
}

}  // namespace network::url
