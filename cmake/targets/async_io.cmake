# ============================================================
# EngineBuilderAsyncIO static library
# ============================================================

add_library(EngineBuilderAsyncIO STATIC ${ENGINE_ASYNC_IO_CC})
target_sources(EngineBuilderAsyncIO PRIVATE ${ENGINE_ASYNC_IO_H})
target_include_directories(EngineBuilderAsyncIO PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
