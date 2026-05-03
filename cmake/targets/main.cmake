# ============================================================
# EngineBuilder main executable
# ============================================================

add_executable(XER ${ENGINE_PROJECT_CC})
target_sources(XER PRIVATE ${ENGINE_PROJECT_H})

target_include_directories(XER PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor
    ${LIBXML2_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
    ${NODE_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    ${LLVM_DEFINITIONS})

if(ENGINE_HAS_ZSTD)
    target_include_directories(XER PRIVATE ${ZSTD_INCLUDE_DIR})
endif()
if(ENGINE_HAS_BROTLI)
    target_include_directories(XER PRIVATE ${BROTLI_INCLUDE_DIR})
endif()
if(ENGINE_HAS_FFI)
    target_include_directories(XER PRIVATE ${FFI_INCLUDE_DIR})
endif()
if(ENGINE_HAS_FFTW3F)
    target_include_directories(XER PRIVATE ${FFTW3_INCLUDE_DIR})
endif()
if(ENGINE_HAS_ICU)
    target_include_directories(XER PRIVATE ${ICU_INCLUDE_DIRS})
endif()

target_compile_definitions(XER PRIVATE
    ${LLVM_DEFINITIONS}
    ENGINE_HAS_ZSTD=${ENGINE_HAS_ZSTD}
    ENGINE_HAS_BROTLI=${ENGINE_HAS_BROTLI}
    ENGINE_HAS_FFI=${ENGINE_HAS_FFI}
    ENGINE_HAS_FFTW3F=${ENGINE_HAS_FFTW3F}
    ENGINE_HAS_ICU=${ENGINE_HAS_ICU}
    ENGINE_HAS_SQLITE3=${ENGINE_HAS_SQLITE3}
    ENGINE_HAS_QT6=${ENGINE_HAS_QT6})

# ── Required link deps ───────────────────────────────────────
target_link_libraries(XER PRIVATE
    XERAsyncIO
    XERJavaScript
    ${NODE_LIBRARY}
    CURL::libcurl
    OpenSSL::Crypto
    ZLIB::ZLIB
    absl::strings
    absl::str_format
    absl::hash
    Eigen3::Eigen
    ${LLVM_ALL_COMPONENT_LIBS}
    ${CLANG_AVAILABLE_LIBS})

if(CMAKE_DL_LIBS)
    target_link_libraries(XER PRIVATE ${CMAKE_DL_LIBS})
endif()

# ── Optional system libs ─────────────────────────────────────
if(ENGINE_HAS_ZSTD)
    target_link_libraries(XER PRIVATE ${ZSTD_LIBRARY})
endif()

if(ENGINE_HAS_BROTLI)
    target_link_libraries(XER PRIVATE
        ${BROTLI_ENC_LIBRARY}
        ${BROTLI_DEC_LIBRARY}
        ${BROTLI_COMMON_LIBRARY})
endif()

if(ENGINE_HAS_FFI)
    target_link_libraries(XER PRIVATE ${FFI_LIBRARY})
endif()

if(ENGINE_HAS_FFTW3F)
    target_link_libraries(XER PRIVATE ${FFTW3F_LIBRARY})
endif()

if(ENGINE_HAS_ICU)
    target_link_libraries(XER PRIVATE ${ICU_LIBRARIES})
endif()

if(TARGET LibXml2::LibXml2)
    target_link_libraries(XER PRIVATE LibXml2::LibXml2)
else()
    target_link_libraries(XER PRIVATE ${LIBXML2_LIBRARIES})
endif()

if(SQLite3_FOUND)
    target_link_libraries(XER PRIVATE SQLite::SQLite3)
endif()

# libuv
if(libuv_FOUND AND TARGET libuv::libuv)
    target_link_libraries(XER PRIVATE libuv::libuv)
else()
    pkg_check_modules(LIBUV libuv QUIET)
    if(LIBUV_FOUND)
        target_include_directories(XER PRIVATE ${LIBUV_INCLUDE_DIRS})
        target_link_libraries(XER PRIVATE ${LIBUV_LIBRARIES})
    endif()
endif()

# jsoncpp
if(TARGET JsonCpp::JsonCpp)
    target_link_libraries(XER PRIVATE JsonCpp::JsonCpp)
elseif(TARGET jsoncpp_lib)
    target_link_libraries(XER PRIVATE jsoncpp_lib)
else()
    pkg_check_modules(JSONCPP jsoncpp QUIET)
    if(JSONCPP_FOUND)
        target_include_directories(XER PRIVATE ${JSONCPP_INCLUDE_DIRS})
        target_link_libraries(XER PRIVATE ${JSONCPP_LIBRARIES})
    endif()
endif()

# re2
if(TARGET re2::re2)
    target_link_libraries(XER PRIVATE re2::re2)
else()
    pkg_check_modules(RE2 re2 QUIET)
    if(RE2_FOUND)
        target_include_directories(XER PRIVATE ${RE2_INCLUDE_DIRS})
        target_link_libraries(XER PRIVATE ${RE2_LIBRARIES})
    endif()
endif()

# range-v3
if(TARGET range-v3::range-v3)
    target_link_libraries(XER PRIVATE range-v3::range-v3)
elseif(TARGET range-v3)
    target_link_libraries(XER PRIVATE range-v3)
else()
    pkg_check_modules(RANGEV3 range-v3 QUIET)
    if(RANGEV3_FOUND)
        target_include_directories(XER PRIVATE ${RANGEV3_INCLUDE_DIRS})
        target_link_libraries(XER PRIVATE ${RANGEV3_LIBRARIES})
    endif()
endif()

if(TARGET absl::flat_hash_map)
    target_link_libraries(XER PRIVATE absl::flat_hash_map)
endif()

# ── Bridge static libs ───────────────────────────────────────
foreach(_bridge XERQt6Bridge XERJavaScript
                XEROpenCV XERBridgeCuda XERBridgeCudnn
                XERBridgeSkia XERBridgeFfmpeg XERBridgeAngle)
    if(TARGET ${_bridge})
        target_link_libraries(XER PRIVATE ${_bridge})
    endif()
endforeach()

if(Qt6Core_FOUND)
    target_link_libraries(XER PRIVATE Qt6::Core)
endif()

# ── Interface alias for all Qt6 targets ──────────────────────
if(ENGINE_QT6_FOUND_TARGETS)
    add_library(EngineQt6All INTERFACE)
    target_link_libraries(EngineQt6All INTERFACE ${ENGINE_QT6_FOUND_TARGETS})
endif()

add_library(EngineClapSmokePlugin MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_app/clap_smoke_plugin.cc)
target_include_directories(EngineClapSmokePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(EngineClapSmokePlugin PROPERTIES
    PREFIX ""
    SUFFIX ".clap")

add_executable(FluxTerminalManagerSmoke
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_app/terminal_manager_smoke.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_interface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_buffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_emulator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_output_renderer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_ansi_parser.cc)
target_include_directories(FluxTerminalManagerSmoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(FluxTerminalAnsiSmoke
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_app/terminal_ansi_smoke.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_interface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_buffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_emulator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_output_renderer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_ansi_parser.cc)
target_include_directories(FluxTerminalAnsiSmoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(FluxCoreSmoke
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_app/flux_core_smoke.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/helper/string.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/core/flux_config.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/core/flux_context.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/core/flux_core.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/core/logger.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/core/module_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_interface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_buffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_emulator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_output_renderer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_ansi_parser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_input_handler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_size_detector.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_window.cc)
target_include_directories(FluxCoreSmoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(ENGINE_HAS_ICU)
    target_include_directories(FluxCoreSmoke PRIVATE ${ICU_INCLUDE_DIRS})
endif()
target_link_libraries(FluxCoreSmoke PRIVATE absl::strings)
if(ENGINE_HAS_ICU)
    target_link_libraries(FluxCoreSmoke PRIVATE ${ICU_LIBRARIES})
endif()
if(TARGET range-v3::range-v3)
    target_link_libraries(FluxCoreSmoke PRIVATE range-v3::range-v3)
elseif(TARGET range-v3)
    target_link_libraries(FluxCoreSmoke PRIVATE range-v3)
endif()

add_executable(EngineDoctorCoreSmoke
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_app/doctor_core_analysis_smoke.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_config.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/event_bus.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/logger.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_context.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/module_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_core.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/scanner/scanner_module.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/data_provider.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_strategy.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_result_handler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/metric_calculator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_task_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_pipeline.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_engine.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_interface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_buffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_emulator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_output_renderer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_ansi_parser.cc)
target_include_directories(EngineDoctorCoreSmoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor)

add_executable(FluxUiSmoke
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_app/flux_ui_smoke.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/history/action_history.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/history/command_history.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/input/command_parser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/input/input_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/input/key_binding_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/rendering/frame_buffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/rendering/render_context.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/rendering/renderer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/rendering/render_pipeline.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_interface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_buffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_manager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_emulator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_output_renderer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_ansi_parser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_input_handler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_size_detector.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flux/terminal/terminal_window.cc)
target_include_directories(FluxUiSmoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
