# async_io

Low-level async and buffered I/O primitives used by higher-level modules and core utility layer.

## Main components

- `async_file_reader.*` — async/sync file read operations
- `async_file_writer.*` — async/sync file write operations
- `io_buffer_pool.*` — reusable byte buffer pool
- `io_cache_manager.*` — in-memory cache with eviction
- `io_thread_pool.*` — shared thread pool
- `io_stream_reader.*` / `io_stream_writer.*`
- `io_file_map.*`
- `io_prefetch_manager.*`

## Project integration

`async_io` is used in:

- `src/modules/io_async_module.cc`
- `src/tool_to.cc`
- `src/flow_script.cc` (through ToolTo)
- `src/runtime_live_interface_compiler.cc` (through ToolTo)
- `src/compiler_api.cc` (through ToolTo)
