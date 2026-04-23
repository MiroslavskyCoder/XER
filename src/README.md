# src

Core engine implementation for AudioPerfX.

## Main areas

- compiler pipeline:
  - `compiler_source.*`
  - `compiler.*`
  - `compiler_api.*`
- runtime execution:
  - `flow_script.*`
  - `runtime_live_*`
- helper utility layer:
  - `tool_to.*`
  - `stack_error.*`
- module system:
  - `modules/`
- async I/O internals:
  - `async_io/`

## Notes

- RuntimeLive compile path uses Clang/LLVM in-process compilation logic.
- Runtime binary execution writes output to generated runtime files and is surfaced via callbacks.
