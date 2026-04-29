# Modules

This directory contains JavaScript modules imported at runtime with:

```javascript
ImportModule("ModuleName")
```

## Detailed reference

Full per-module description and API list: 

## Quick map

- `FileSystem` — path/file operations
- `RuntimeLive` — runtime compiler path/command helpers
- `Container` — full `src/` source-tree indexing, search, and file reads
- `Provider` — provider key/value store helpers via `Provider.ProviderStore`
- `Util` — path normalization and helper utilities

## Implemented now

- `ImportModule("Container")`
	- `Container.sourceRoot()`
	- `Container.listSourceFiles([relativeDir])`
	- `Container.listSourceDirectories([relativeDir])`
	- `Container.readSourceFile(relativePath)`
	- `Container.findSourceFiles(query)`
	- `Container.describeSourceTree([relativeDir])`
	- `Container.availableModules()`
- `ImportModule("FileSystem")`
	- `FileSystem.exists(path)`
	- `FileSystem.isFile(path)`
	- `FileSystem.isDirectory(path)`
	- `FileSystem.readText(path)`
	- `FileSystem.writeText(path, text)`
	- `FileSystem.createDirectories(path)`
	- `FileSystem.listDir([path])`
- `ImportModule("Util")`
	- `Util.normalizePath(path)`
	- `Util.joinPath(...segments)`
	- `Util.basename(path)`
	- `Util.dirname(path)`
	- `Util.extension(path)`
	- `Util.cwd()`
	- `Util.sourceRoot()`
- `ImportModule("Provider")`
	- `Provider.create([initialObject])`
	- `new Provider.ProviderStore([initialObject])`
	- `ProviderStore.set(key, value)`
	- `ProviderStore.get(key[, fallback])`
	- `ProviderStore.keys()`
- `ImportModule("RuntimeLive")`
	- `RuntimeLive.defaultProvider()`
	- `RuntimeLive.describeCompiler(source[, options])`
	- `RuntimeLive.buildCompilerCommandPreview(source[, options[, compilerBinary]])`

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
- `VTK`

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