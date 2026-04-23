# ============================================================
# EngineJavaScript static library  (V8 / libuv / ICU)
# ============================================================

if(NOT ENGINE_JAVASCRIPT_CC)
    return()
endif()

add_library(EngineJavaScript STATIC ${ENGINE_JAVASCRIPT_CC})
target_sources(EngineJavaScript PRIVATE ${ENGINE_JAVASCRIPT_H})
target_include_directories(EngineJavaScript PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${NODE_INCLUDE_DIR})

target_compile_definitions(EngineJavaScript PRIVATE
    ENGINE_HAS_V8=1
    ENGINE_HAS_LIBUV=1
    ENGINE_HAS_ICU=${ENGINE_HAS_ICU})

# ── V8 ───────────────────────────────────────────────────────
if(V8_FOUND AND TARGET V8::V8)
    target_link_libraries(EngineJavaScript PRIVATE V8::V8)
else()
    pkg_check_modules(V8 libv8 QUIET)
    if(V8_FOUND)
        target_include_directories(EngineJavaScript PRIVATE ${V8_INCLUDE_DIRS})
        target_link_libraries(EngineJavaScript PRIVATE ${V8_LIBRARIES})
    endif()
endif()

# ── libuv ────────────────────────────────────────────────────
if(libuv_FOUND AND TARGET libuv::libuv)
    target_link_libraries(EngineJavaScript PRIVATE libuv::libuv)
else()
    pkg_check_modules(LIBUV libuv QUIET)
    if(LIBUV_FOUND)
        target_include_directories(EngineJavaScript PRIVATE ${LIBUV_INCLUDE_DIRS})
        target_link_libraries(EngineJavaScript PRIVATE ${LIBUV_LIBRARIES})
    endif()
endif()

# ── Common deps ──────────────────────────────────────────────
target_link_libraries(EngineJavaScript PRIVATE
    absl::strings
    absl::str_format
    absl::hash
    ZLIB::ZLIB)

if(ICU_FOUND)
    target_include_directories(EngineJavaScript PRIVATE ${ICU_INCLUDE_DIRS})
    target_link_libraries(EngineJavaScript PRIVATE ${ICU_LIBRARIES})
endif()
