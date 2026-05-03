/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/http/http_cache.h"

namespace network::http {

HttpCache& HttpCache::Instance() {
    static HttpCache inst;
    return inst;
}

bool HttpCache::Lookup(const std::string& key, CachedResponse* out) const {
    std::lock_guard<std::mutex> lk(mu_);
    const auto it = store_.find(key);
    if (it == store_.end()) return false;
    if (std::chrono::steady_clock::now() > it->second.expires) return false;
    if (out) *out = it->second;
    return true;
}

void HttpCache::Store(const std::string& key, CachedResponse entry) {
    std::lock_guard<std::mutex> lk(mu_);
    store_[key] = std::move(entry);
}

void HttpCache::Invalidate(const std::string& key) {
    std::lock_guard<std::mutex> lk(mu_);
    store_.erase(key);
}

void HttpCache::Clear() {
    std::lock_guard<std::mutex> lk(mu_);
    store_.clear();
}

}  // namespace network::http
