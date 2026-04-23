#pragma once

#include <string>
#include <vector>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::web {

struct WebRequestResult {
    bool        ok       = false;
    int         status   = 0;
    std::string body;
    std::string error;
    std::string content_type;
};

struct WebProfileOptions {
    std::string name;                // profile name / "Default"
    std::string cache_path;          // override cache dir
    std::string storage_path;        // override persistent storage dir
    bool        off_the_record = false;
};

// Load a URL to string using QWebEngineView (headless-like, requires event loop)
// For scripting use: returns immediately with request id — callback-based.
// Simpler sync helper that blocks until load is done (GUI thread use only):
WebRequestResult LoadUrlSync(const std::string& url, int timeout_ms = 10000);

// Check if WebEngine module is available at runtime
bool IsWebEngineAvailable();

// Get user agent string
std::string GetUserAgent();

// Profile helpers
std::string GetDefaultCachePath();
std::string GetDefaultStoragePath();

}  // namespace qt6::web
