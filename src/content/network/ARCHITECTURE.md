# Network Module Architecture

## Overview

The `src/content/network` module is a comprehensive, production-grade network stack inspired by Google Chromium's architecture. It provides:

- **Full HTTP/1.1 + HTTP/2 support** with keep-alive and connection pooling
- **HTTP/3 (QUIC)** for UDP-based, low-latency transport
- **WebSocket** for real-time bidirectional communication
- **HTTPS/TLS** with certificate verification
- **DNS resolution** with caching and async support
- **Proxy support** (HTTP, HTTPS, SOCKS4/5)
- **FTP** for file transfers
- **SPDY** for multiplexed streaming

The module is built on an **asynchronous, event-driven architecture** using thread pools and callback-based APIs to avoid blocking operations.

## Architecture Layers

```
┌─────────────────────────────────────────────┐
│         Application / JavaScript            │
├─────────────────────────────────────────────┤
│ URLRequest + URLRequestContext (HTTP API)   │
├─────────────────────────────────────────────┤
│ HttpNetworkSession (Protocol Handlers)      │
│    ├─ HttpStream (HTTP/1.1, HTTP/2)        │
│    ├─ WebSocketStream (WebSocket)          │
│    ├─ QuicStream (QUIC/HTTP3)              │
│    ├─ SpdyStream (SPDY)                    │
│    └─ FtpStream (FTP)                      │
├─────────────────────────────────────────────┤
│ Transport + Socket Layer                    │
│    ├─ TcpClientSocket / SslClientSocket    │
│    ├─ UdpClientSocket                      │
│    └─ SocketProxy (for SOCKS/CONNECT)      │
├─────────────────────────────────────────────┤
│ Supporting Services                         │
│    ├─ HostResolver (DNS)                   │
│    ├─ ProxyService                         │
│    ├─ CertVerifier (SSL/TLS)               │
│    └─ HttpCache (response caching)         │
├─────────────────────────────────────────────┤
│ Operating System / Network (Async I/O)      │
└─────────────────────────────────────────────┘
```

## Module Organization

### `core/`
Central context and service definitions:
- `NetworkContext` — singleton aggregating all subsystems
- `NetworkService` — high-level service interface
- `NetworkServiceDelegate` — observer pattern for events

### `url/`
URL handling and request execution:
- `URLRequest` / `URLResponse` — request/response structures
- `URLRequestContext` — per-request configuration (cookies, proxy, auth, etc.)
- `URLRequestJob` — synchronous request executor
- `URLRequestInterceptingJob` — hook for mocking/caching

### `http/`
HTTP protocol implementation:
- `HttpNetworkSession` — manages HTTP/1.1 and HTTP/2 connections
- `HttpStream` — abstract protocol stream
- `HttpStreamFactory` — creates protocol-specific streams
- `HttpRequestHeaders` / `HttpResponseHeaders` — header handling
- `HttpAuth` — authentication (Basic, Digest, NTLM, Negotiate)
- `HttpCache` — response caching
- `HttpServerProperties` — ALPN, alt-svc, known hosts

### `websocket/`
WebSocket protocol (RFC 6455):
- `WebSocketStream` — full-duplex stream with framing
- `WebSocketHandshakeStream` — upgrade handshake
- `WebSocketFrame` — frame encoding/decoding
- `WebSocketEventInterface` — callbacks (OnDataFrame, OnClosing, etc.)

### `quic/`
QUIC protocol (RFC 9000):
- `QuicStream` — multiplexed streams within a connection
- `QuicConnection` — endpoint managing streams
- `QuicProtocol` — protocol constants and helpers
- `QuicVersionManager` — version negotiation
- `QuicCryptoStream` — TLS 1.3 over QUIC

### `spdy/`
SPDY protocol (predecessor to HTTP/2):
- `SpdyStream` — multiplexed stream
- `SpdySession` — connection multiplexing
- `SpdyFramer` — frame parsing/serialization
- `SpdyProtocol` — SPDY 3.1 constants

### `socket/`
Low-level socket operations:
- `TcpClientSocket` / `SocketServer` — TCP client and server
- `UdpClientSocket` — UDP datagram socket
- `SslClientSocket` — TLS/SSL over TCP
- `SocketStream` — buffered I/O wrapper
- `SocketProxy` — HTTP CONNECT and SOCKS tunneling
- `NetworkSelector` — proxy and socket type selection

### `transport/`
Transport-level abstractions:
- `TransportClientSocket` — abstract client interface
- `TransportServerSocket` — abstract server interface
- `TransportNetworkDelegate` — observer for I/O events
- `TransportNetworkSession` — concrete POSIX TCP implementation
- `TransportIoBuffer` — buffer management

### `proxy/`
Proxy configuration and handling:
- `ProxyConfig` — configuration for proxies and bypass rules
- `ProxyService` — service for finding applicable proxy
- `ProxyResolver` — PAC file handling
- `ProxyInfo` — Direct() or Via() helpers
- `SocksClientSocket` — SOCKS4/5 client implementation

### `cert/`
SSL/TLS certificate management:
- `CertVerifier` — X509 certificate verification
- `CertStorage` — PEM bundle loading and caching
- `CertDatabase` — certificate lookup with expiration
- `CertTransparency` — SCT log verification
- `ScopedNssTypes` — RAII wrappers for OpenSSL/NSS objects

### `dns/`
Domain Name System resolution:
- `HostResolver` — async DNS hostname lookup with cache
- `DnsResolver` — high-level interface
- `DnsClient` — low-level UDP protocol
- `DnsConfigService` — reads `/etc/resolv.conf`
- `DnsRecordType` — A, AAAA, CNAME, MX, SRV, etc.

### `ftp/`
FTP protocol support:
- `FtpStream` — FTP command/data channel handling
- `FtpRequest` — FTP transfer configuration
- `FtpNetworkDelegate` — observer

### `network_util/`
Utility functions:
- IP address parsing and formatting (IPv4, IPv6)
- URL encoding/decoding
- Header parsing
- Standard port lookup

## Key Design Patterns

### 1. Asynchronous I/O
All network operations are non-blocking:
- Synchronous methods return success/error inline
- Asynchronous methods accept a callback
- Background work uses `IOThreadPool::GetSharedInstance().Enqueue()`

```cpp
// Sync
int n = socket.Read(buf, len, &error);

// Async
socket.ReadAsync(buf, len, [](int n) { /* callback */ });
```

### 2. Observer Pattern
Network events are reported via delegates:
- `TransportNetworkDelegate` — byte counts, connections, errors
- `HttpNetworkDelegate` — HTTP-specific events
- `WebSocketEventInterface` — WebSocket frame events
- `ProxyService::Observer` — proxy changes

### 3. Error Handling
All errors are reported via output parameters:
- Synchronous: `std::string* error` parameter
- Asynchronous: error string in callback

```cpp
bool Connect(const std::string& host, uint16_t port, std::string* error);
// error is filled on failure (return value = false)
```

### 4. Resource Management
- RAII for socket file descriptors
- Shared pointers for buffers (`IOBuffer`)
- Thread-safe singletons (double-checked locking)

### 5. Protocol Layering
Each protocol implements a common interface:
- `HttpStream` — base class for HTTP/1.1, HTTP/2, QUIC
- `TransportClientSocket` — base class for TCP, SSL, UDP
- Protocol-specific details hidden in implementations

## Usage Examples

### HTTP GET Request
```cpp
#include "src/content/network/url/url_request_util.h"

// High-level: synchronous GET
std::string response = network::url::SyncGet("http://example.com/api");
```

### WebSocket Connection
```cpp
#include "src/content/network/websocket/websocket_stream.h"

network::websocket::WebSocketStream ws;
std::string error;

// Handshake
if (!ws.DoHandshake("ws://echo.websocket.org", {}, &error)) {
    // handle error
}

// Send message
ws.SendText("Hello", [](bool ok) { /* callback */ });

// Receive
ws.OnDataFrameReceived = [](const WebSocketFrame& frame) {
    if (frame.opcode == WebSocketOpcode::kText) {
        // process text message
    }
};
```

### Custom Socket Connection
```cpp
#include "src/content/network/socket/tcp_client_socket.h"

network::socket::TcpClientSocket socket;
std::string error;

if (socket.Connect("example.com", 443, &error)) {
    uint8_t buf[1024];
    int n = socket.Read(buf, sizeof(buf), &error);
    // process data
    socket.Disconnect();
}
```

### DNS Lookup
```cpp
#include "src/content/network/dns/dns_resolver.h"

auto& resolver = network::dns::DnsResolver::Instance();
std::vector<std::string> ips;
std::string error;

if (resolver.Resolve("example.com", &ips, &error)) {
    for (const auto& ip : ips) {
        // use IP address
    }
}
```

## Thread Safety

- **Main thread**: all synchronous APIs
- **Thread pool**: async callbacks run on `IOThreadPool`
- **Singletons**: thread-safe via mutex or atomic patterns
- **Buffers**: `IOBuffer` is thread-safe for reference counting

## Async Model

```cpp
// High-level async pattern
socket.ReadAsync(buf, len, [callback](int bytes_read) {
    if (bytes_read > 0) {
        // Success - process data
    } else if (bytes_read == 0) {
        // EOF
    } else {
        // Error
    }
});

// Implementation dispatches to thread pool
IOThreadPool::GetSharedInstance().Enqueue([=]() {
    std::string error;
    int n = socket.Read(buf, len, &error);
    // Invoke callback from thread pool thread
    callback(n);
});
```

## Configuration

Through `URLRequestContext`:
```cpp
// Set proxy
context.proxy_config.proxies["http"] = "http://proxy.example.com:8080";

// Set timeout
context.socket_timeout_ms = 60000;

// Enable SSL verification
context.skip_ssl_verify = false;

// Set user agent
context.user_agent = "XER/1.0";
```

## Error Handling Strategy

The module uses a three-tier error strategy:

1. **Connection errors** — reported to TransportNetworkDelegate
2. **Protocol errors** — logged to NetLog, reported via callbacks
3. **I/O errors** — included in completion callbacks

Never throws exceptions; all errors reported via output parameters or callbacks.

## Performance Considerations

- **Connection pooling** — HTTP/1.1 sockets are reused
- **Multiplexing** — HTTP/2, QUIC, SPDY share single connection
- **DNS caching** — entries cached with TTL
- **HTTP caching** — responses cached per Cache-Control
- **Buffer pooling** — `IOBufferPool` reuses allocations
- **Async I/O** — prevents main thread blocking

## Security Features

- **Certificate verification** — X509 validation with CA bundle
- **TLS 1.2/1.3** — via OpenSSL
- **HSTS** — enforced via HttpServerProperties
- **CORS** — respected (app enforces)
- **CSRF tokens** — app responsible for validation

## Testing

Use smoke tests in `demo_app/`:
```bash
./out/build/default/EngineBuilder run demo_app/bridge_multimedia.js
```

See [src/content/network/README.md](README.md) (Russian) for module-specific documentation.
