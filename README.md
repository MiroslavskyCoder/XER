# XER 🚀

AudioPerfX is a high-performance C++/V8 runtime builder that executes JavaScript project configurations and compiles native code dynamically through `RuntimeLive`. It provides a powerful bridge between JavaScript and advanced C++ multimedia/GPU frameworks.

## ✨ Key Features

- **JavaScript-Driven Build Flows:** Orchestrate builds easily via `EngineBuilder`.
- **Dynamic Native Compilation:** Real-time C/C++ compilation with robust Clang/LLVM integration via `RuntimeLive`.
- **Multimedia & GPU Bridges:** Built-in wrappers for Skia, FFmpeg, OpenCV, CUDA, cuDNN, and ANGLE.
- **Rich Module System:** Use `ImportModule("...")` to access the filesystem, cryptography, compression, networking, and asynchronous I/O helpers.
- **In-Process Compile Path:** Seamless Clang/LLVM integration during the `RuntimeLive` compile stage.

## 📦 Project Layout

- `src/` — Core runtime engine and compiler bridge.
- `src/modules/` — Standard JavaScript modules available for import.
- `src/async_io/` — Asynchronous I/O primitives for modules and core utilities.
- `demo_app/` — Active multimedia bridge demo configurations and scripts.

## 🛠️ Build Instructions

### Prerequisites
Make sure you have CMake and a compatible C++ compiler (like Clang/GCC) installed. You may also need dependencies for OpenSSL, Zlib, Zstd, and Brotli.

### Compilation
```bash
# Configure the project
cmake -S . -B build

# Build the executable (using 4 threads)
cmake --build build -j4
```

## 🚀 Quick Start

Run the multimedia bridge demo (showcases Skia, FFmpeg, OpenCV, etc., via JS):

```bash
./build/EngineBuilder run demo_app/bridge_multimedia.js
```

## 🔒 Crypto & Compression Support

AudioPerfX includes built-in support for multiple algorithms:
- **Crypto:** OpenSSL `libcrypto` integration (`Crypto` & `Bcrypto`).
- **Compression:** 
  - `Zlib` (standard)
  - `Zstd` (requires `libzstd` dev package)
  - `Brotli` (requires `libbrotli` dev package)

## 📚 Documentation

Detailed documentation and examples can be found in the following locations:
- [Demo App Docs](demo_app/README.md)
- [Examples Docs](example/README.md)
- [JavaScript Modules Docs](src/modules/README.md)
- [Async I/O Docs](src/async_io/README.md)
- [Module Reference](docs/modules.md)
- [Stack Error Handling](docs/stack_error.md)
