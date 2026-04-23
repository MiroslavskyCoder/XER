# AudioPerfX

AudioPerfX is a C++/V8 runtime builder that executes JavaScript project configs and compiles native code through RuntimeLive.

## What this project provides

- JavaScript-driven build flows via `EngineBuilder`
- Runtime C/C++ compilation with Clang/LLVM integration
- Rich module system (`ImportModule("...")`) for filesystem, runtime, crypto, compression, networking, async helpers, and more
- In-process compile path for Clang/LLVM in RuntimeLive compile stage

## Build

```bash
cmake -S . -B build
cmake --build build -j4
```

## Run
 
Bridge demo (new multimedia/GPU wrappers):

```bash
./build/EngineBuilder run demo_app/bridge_multimedia.js
```
 
## Project layout

- `src/` — core runtime and compiler bridge
- `src/modules/` — importable JavaScript modules
- `src/async_io/` — async I/O primitives used by modules and core utility layer
- `demo_app/` — active demo configuration 

## Compression and crypto support

The project includes:

- OpenSSL libcrypto integration for `Crypto` and `Bcrypto`
- `Zlib` module (`zlib`)
- `Zstd` module (when `zstd` dev package is available)
- `Brotli` module (when brotli dev package is available)

## Docs map

- `demo_app/README.md`
- `example/README.md`
- `src/modules/README.md`
- `src/async_io/README.md`
- `docs/modules.md`
- `docs/stack_error.md`
