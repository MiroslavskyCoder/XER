# ============================================================
# Source file globs + filters
# ============================================================

file(GLOB_RECURSE ENGINE_ASYNC_IO_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/async_io/*.cc")
file(GLOB_RECURSE ENGINE_ASYNC_IO_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/async_io/*.h")

file(GLOB_RECURSE ENGINE_SRC_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
file(GLOB_RECURSE ENGINE_SRC_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

file(GLOB_RECURSE ENGINE_QT6_SUPPORT_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/qt6/*.cc")
file(GLOB_RECURSE ENGINE_QT6_SUPPORT_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/qt6/*.h")

file(GLOB_RECURSE ENGINE_QT6_MODULE_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/qt6/*.cc")
file(GLOB_RECURSE ENGINE_QT6_MODULE_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/qt6/*.h")

file(GLOB_RECURSE ENGINE_JAVASCRIPT_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/javascript/*.cc")
file(GLOB_RECURSE ENGINE_JAVASCRIPT_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/javascript/*.h")

file(GLOB_RECURSE ENGINE_WRAPPER_OPENCV_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/opencv/*.cc")
file(GLOB_RECURSE ENGINE_WRAPPER_OPENCV_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/opencv/*.h")

file(GLOB_RECURSE ENGINE_WRAPPER_CUDA_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/cuda/*.cc")
file(GLOB_RECURSE ENGINE_WRAPPER_CUDA_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/cuda/*.h")

file(GLOB_RECURSE ENGINE_WRAPPER_CUDNN_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/cudnn/*.cc")
file(GLOB_RECURSE ENGINE_WRAPPER_CUDNN_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/cudnn/*.h")

file(GLOB_RECURSE ENGINE_WRAPPER_SKIA_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/skia/*.cc")
file(GLOB_RECURSE ENGINE_WRAPPER_SKIA_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/skia/*.h")

file(GLOB_RECURSE ENGINE_WRAPPER_FFMPEG_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/ffmpeg/*.cc")
file(GLOB_RECURSE ENGINE_WRAPPER_FFMPEG_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/ffmpeg/*.h")

file(GLOB_RECURSE ENGINE_WRAPPER_ANGLE_CC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/angle/*.cc")
file(GLOB_RECURSE ENGINE_WRAPPER_ANGLE_H CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/angle/*.h")

# Qt6 combined
set(ENGINE_QT6_PROJECT_CC ${ENGINE_QT6_SUPPORT_CC} ${ENGINE_QT6_MODULE_CC})
set(ENGINE_QT6_PROJECT_H  ${ENGINE_QT6_SUPPORT_H}  ${ENGINE_QT6_MODULE_H})

# CUDA sources
file(GLOB_RECURSE ENGINE_SRC_CU CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cu")

# Core sources — strip sub-trees that belong to separate static libs
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/wrapper/qt6/")
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/modules/qt6/")
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/async_io/")
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/javascript/")
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/wrapper/")
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/doctor/")
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/content/ai/ml/data_augmentation/")
list(FILTER ENGINE_SRC_CC EXCLUDE REGEX "/src/v8/v8_runtime_checker\\.cc")
list(FILTER ENGINE_SRC_CU EXCLUDE REGEX "/src/wrapper/")
list(APPEND ENGINE_SRC_CC "${CMAKE_CURRENT_SOURCE_DIR}/src/xer/encode/sxer84321.c")

list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/wrapper/qt6/")
list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/modules/qt6/")
list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/async_io/")
list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/javascript/")
list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/wrapper/")
list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/doctor/")
list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/content/ai/ml/data_augmentation/")
list(FILTER ENGINE_SRC_H EXCLUDE REGEX "/src/v8/v8_runtime_checker\\.h$")

set(ENGINE_DOCTOR_RUNTIME_CC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_config.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/event_bus.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/logger.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_context.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/module_manager.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_core.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/scanner/scanner_module.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/data_provider.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_strategy.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_result_handler.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/metric_calculator.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_task_manager.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_pipeline.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_engine.cc")

set(ENGINE_DOCTOR_RUNTIME_H
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_config.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/event_bus.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/exceptions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/logger.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_context.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/module_manager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/core/engine_doctor_core.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/scanner/scan_parameters.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/scanner/scan_result.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/scanner/scanner_module.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/data_provider.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_strategy.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_result_handler.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/metric_calculator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_task_manager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_pipeline.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/analysis/analysis_engine.h")

set(ENGINE_PROJECT_CC ${ENGINE_SRC_CC} ${ENGINE_DOCTOR_RUNTIME_CC} ${ENGINE_SRC_CU})
set(ENGINE_PROJECT_H  ${ENGINE_SRC_H} ${ENGINE_DOCTOR_RUNTIME_H})
