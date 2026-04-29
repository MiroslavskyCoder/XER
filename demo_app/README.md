# demo_app

## Bridge multimedia demo

Run bridge status check for multimedia/GPU related modules:

```bash
./build/EngineBuilder run demo_app/bridge_multimedia.js
```

Run the broader module-registry smoke check used after refactors:

```bash
./out/build/default/EngineBuilder run demo_app/modules_smoke.js
```

Run the FlowScript Buffer smoke check:

```bash
./out/build/default/EngineBuilder run demo_app/buffer_smoke.js
```

Run the FlowScript console smoke check:

```bash
./out/build/default/EngineBuilder run demo_app/console_smoke.js
```

Run the terminal snapshot smoke check:

```bash
./out/build/default/EngineBuilder run demo_app/terminal_snapshot_smoke.js
```

Run the standalone C++ terminal manager smoke check:

```bash
./out/build/default/FluxTerminalManagerSmoke
```

Run the standalone ANSI/renderer smoke check:

```bash
./out/build/default/FluxTerminalAnsiSmoke
```

This script checks these modules:

- OpenCV
- CUDA
- CUDNN
- Skia
- FFmpeg
- ANGLE

The module smoke script also verifies:

- Container
- System
- RuntimeLive
- Doctor
- VTK

The Buffer smoke script verifies:

- global `Buffer` binding
- `Buffer.from/alloc/concat/byteLength/isBuffer`
- `toString`, `copy`, `slice`, `fill`
- `indexOf`, `lastIndexOf`
- `swap16`, `swap32`, `swap64`

The console smoke script verifies:

- global `Console` class and `console` instance
- `log`, `info`, `warn`, `error`, `dir`, `assert`
- object, array, and Buffer formatting through the new `src/flux` console runtime

The terminal snapshot smoke script verifies:

- direct `console.snapshot()` access to the `src/flux/terminal` buffer state
- direct `console.clearSnapshot()` reset behavior for stdout and stderr buffers

The standalone C++ terminal manager smoke verifies:

- direct `TerminalManager::Write`, `WriteLine`, `Snapshot`, and `ResetBuffer` behavior
- stdout and stderr buffer separation without using FlowScript runtime

The standalone ANSI/renderer smoke verifies:

- raw ANSI escape sequences remain in the terminal snapshot buffer
- `SnapshotPlainText()` now runs through the terminal emulator layer instead of using parser output directly
- renderer-facing plain text strips CSI, OSC, DCS, APC, PM, SOS, and single ESC control paths
- stdout clearing does not wipe stderr renderer state
