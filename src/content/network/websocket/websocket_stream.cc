/**
 * This file is part of XER, Network open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/network/websocket/websocket_stream.h"

#include <vector>

namespace network::websocket {

WebSocketStream::WebSocketStream(
    std::unique_ptr<transport::TransportClientSocket> socket)
    : sock_(std::move(socket)), open_(true) {}

WebSocketStream::~WebSocketStream() {
    if (open_) Close();
}

bool WebSocketStream::SendText(const std::string& text, std::string* error) {
    WebSocketFrame f;
    f.opcode  = WebSocketOpcode::kText;
    f.masked  = true;
    f.payload.assign(text.begin(), text.end());
    auto wire = EncodeFrame(f);
    return sock_->Write(wire.data(), wire.size(), error);
}

bool WebSocketStream::SendBinary(const uint8_t* data, size_t len,
                                  std::string* error) {
    WebSocketFrame f;
    f.opcode = WebSocketOpcode::kBinary;
    f.masked = true;
    f.payload.assign(data, data + len);
    auto wire = EncodeFrame(f);
    return sock_->Write(wire.data(), wire.size(), error);
}

bool WebSocketStream::SendPing(std::string* error) {
    WebSocketFrame f;
    f.opcode = WebSocketOpcode::kPing;
    f.masked = true;
    auto wire = EncodeFrame(f);
    return sock_->Write(wire.data(), wire.size(), error);
}

void WebSocketStream::Close(uint16_t code, const std::string& reason) {
    if (!open_) return;
    open_ = false;
    auto close_frame = MakeCloseFrame(code, reason);
    close_frame.masked = true;
    auto wire = EncodeFrame(close_frame);
    std::string err;
    sock_->Write(wire.data(), wire.size(), &err);
    sock_->Disconnect();
}

void WebSocketStream::ReadLoop() {
    std::vector<uint8_t> buf;
    buf.reserve(65536);
    while (open_) {
        uint8_t tmp[4096];
        std::string err;
        const int n = sock_->Read(tmp, sizeof(tmp), &err);
        if (n <= 0) {
            open_ = false;
            if (on_error_ && n < 0) on_error_(err);
            break;
        }
        buf.insert(buf.end(), tmp, tmp + n);
        size_t consumed;
        WebSocketFrame frame;
        while ((consumed = DecodeFrame(buf.data(), buf.size(), &frame)) > 0) {
            buf.erase(buf.begin(), buf.begin() + static_cast<ptrdiff_t>(consumed));
            if (frame.opcode == WebSocketOpcode::kClose) {
                uint16_t code = 1000;
                std::string reason;
                if (frame.payload.size() >= 2) {
                    code = (static_cast<uint16_t>(frame.payload[0]) << 8) |
                            frame.payload[1];
                    reason.assign(frame.payload.begin() + 2, frame.payload.end());
                }
                Close(code, reason);
                if (on_close_) on_close_(code, reason);
                return;
            }
            if (frame.opcode == WebSocketOpcode::kPing) {
                WebSocketFrame pong;
                pong.opcode  = WebSocketOpcode::kPong;
                pong.masked  = true;
                pong.payload = frame.payload;
                std::string we;
                auto wire = EncodeFrame(pong);
                sock_->Write(wire.data(), wire.size(), &we);
                continue;
            }
            if (on_message_) on_message_(frame);
        }
    }
}

}  // namespace network::websocket
