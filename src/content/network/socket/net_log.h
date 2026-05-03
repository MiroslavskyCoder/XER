/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <string>
#include "doctor/core/logger.h"

namespace network::socket {

// Thin adapter: routes socket-layer events to EngineDoctor Logger.
class NetLog {
public:
    static void Debug(const std::string& msg)   { EngineDoctor::Logger::debug("[net] " + msg); }
    static void Info(const std::string& msg)    { EngineDoctor::Logger::info("[net] " + msg); }
    static void Warning(const std::string& msg) { EngineDoctor::Logger::warning("[net] " + msg); }
    static void Error(const std::string& msg)   { EngineDoctor::Logger::error("[net] " + msg); }
};

}  // namespace network::socket
