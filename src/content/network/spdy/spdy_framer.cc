/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/spdy/spdy_framer.h"

namespace network::spdy {

// Minimal SPDY/3 data frame serialization: [stream_id(4)][flags(1)][length(3)][payload]
std::vector<uint8_t> SerializeSpdyFrame(const SpdyFrame& frame) {
    std::vector<uint8_t> out;
    if (!frame.control) {
        // Data frame: first bit 0
        const uint32_t sid = frame.stream_id & 0x7FFFFFFF;
        out.push_back(static_cast<uint8_t>(sid >> 24));
        out.push_back(static_cast<uint8_t>(sid >> 16));
        out.push_back(static_cast<uint8_t>(sid >> 8));
        out.push_back(static_cast<uint8_t>(sid));
        out.push_back(frame.fin ? 0x01 : 0x00);
        const uint32_t len = static_cast<uint32_t>(frame.data.size());
        out.push_back(static_cast<uint8_t>(len >> 16));
        out.push_back(static_cast<uint8_t>(len >> 8));
        out.push_back(static_cast<uint8_t>(len));
        out.insert(out.end(), frame.data.begin(), frame.data.end());
    }
    return out;
}

size_t ParseSpdyFrame(const uint8_t* buf, size_t len, SpdyFrame* out) {
    if (len < 8) return 0;
    const bool ctrl = (buf[0] & 0x80) != 0;
    out->control = ctrl;
    if (!ctrl) {
        out->stream_id = ((uint32_t)(buf[0]&0x7F)<<24)|((uint32_t)buf[1]<<16)|
                          ((uint32_t)buf[2]<<8)|(uint32_t)buf[3];
        out->fin = (buf[4] & 0x01) != 0;
        const uint32_t plen = ((uint32_t)buf[5]<<16)|((uint32_t)buf[6]<<8)|(uint32_t)buf[7];
        if (len < 8 + plen) return 0;
        out->data.assign(buf+8, buf+8+plen);
        return 8 + plen;
    }
    return 0; // control frame parsing stub
}

}  // namespace network::spdy
