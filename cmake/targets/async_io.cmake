# ============================================================
# XERAsyncIO static library
# ============================================================

add_library(XERAsyncIO STATIC ${ENGINE_ASYNC_IO_CC})
target_sources(XERAsyncIO PRIVATE ${ENGINE_ASYNC_IO_H})
target_include_directories(XERAsyncIO PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
