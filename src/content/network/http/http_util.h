/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <string>
#include <map>

namespace network::http {

// Trim whitespace from both ends of |s|.
std::string TrimWhitespace(const std::string& s);

// Case-insensitive ASCII comparison.
bool IEqualASCII(const std::string& a, const std::string& b);

// Split a header field value by |delimiter|, trimming each token.
std::vector<std::string> SplitHeaderValue(const std::string& value,
                                           char delimiter = ',');

// Format a Content-Length value.
std::string FormatContentLength(size_t n);

// Return true if the HTTP method is safe (no side effects).
bool IsMethodSafe(const std::string& method);

// Return true if the HTTP method is idempotent.
bool IsMethodIdempotent(const std::string& method);

// Construct a Reason-Phrase for a status code.
std::string ReasonPhrase(int status_code);

}  // namespace network::http
