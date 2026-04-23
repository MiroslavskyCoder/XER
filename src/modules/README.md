# Modules

This directory contains JavaScript modules imported at runtime with:

```javascript
ImportModule("ModuleName")
```

## Detailed reference

Full per-module description and API list:

- [docs/modules.md](../docs/modules.md)

## Quick map

- `FileSystem` — path/file operations
- `RuntimeLive` — runtime compile and execute interface
- `Container` — source list and path aggregation helpers
- `Provider` — provider configuration helpers
- `Util` — conversion and helper utilities

## Utility/system modules

- `System`
- `Console`
- `Timer`
- `Network`
- `Git`
- `Buffer`
- `SQLite`
- `Qt`

## Multimedia/GPU bridge modules

- `OpenCV`
- `CUDA`
- `CUDNN`
- `Skia`
- `FFmpeg`
- `ANGLE`

## Async and formatting

- `IO/Async` — async helpers + async_io-backed fast file/cache/buffer methods
- `IO/Formating` — string and formatting helpers

## Crypto/compression modules

- `Crypto` — OpenSSL-backed random/hash/base64 operations
- `Bcrypto` — digest/HMAC/PBKDF2 helpers
- `Zlib` — compress/decompress (raw binary string flow)
- `Zstd` — zstd compression support (if available in build env)
- `Brotli` — brotli compression support (if available in build env)

## Registration point

Module dispatch lives in:

- `src/modules/module_registry.cc`
