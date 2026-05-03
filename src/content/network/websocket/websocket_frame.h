/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace network::websocket {

enum class WebSocketOpcode : uint8_t {
    kContinuation = 0x0,
    kText         = 0x1,
    kBinary       = 0x2,
    kClose        = 0x8,
    kPing         = 0x9,
    kPong         = 0xA,
};

struct WebSocketFrame {
    bool            fin     = true;
    bool            masked  = false;
    WebSocketOpcode opcode  = WebSocketOpcode::kText;
    std::vector<uint8_t> payload;
};

// Encode a frame into wire bytes.
std::vector<uint8_t> EncodeFrame(const WebSocketFrame& frame);

// Decode one frame from |data|. Returns bytes consumed, 0 if incomplete.
size_t DecodeFrame(const uint8_t* data, size_t len, WebSocketFrame* frame);

// Build a Close frame with optional status code and reason.
WebSocketFrame MakeCloseFrame(uint16_t code = 1000,
                               const std::string& reason = {});

}  // namespace network::websocket
