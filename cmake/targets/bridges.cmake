# ============================================================
# Optional bridge static libraries
# (OpenCV / CUDA / cuDNN / Skia / FFmpeg / ANGLE)
# ============================================================

# ── OpenCV ───────────────────────────────────────────────────
if(ENGINE_WRAPPER_OPENCV_CC)
    add_library(EngineBridgeOpenCV STATIC ${ENGINE_WRAPPER_OPENCV_CC})
    target_sources(EngineBridgeOpenCV PRIVATE ${ENGINE_WRAPPER_OPENCV_H})
    target_include_directories(EngineBridgeOpenCV PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(EngineBridgeOpenCV PRIVATE
        ENGINE_HAS_OPENCV_BRIDGE=${ENGINE_HAS_OPENCV_BRIDGE})
    if(OpenCV_FOUND)
        target_include_directories(EngineBridgeOpenCV PRIVATE ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(EngineBridgeOpenCV PRIVATE ${OpenCV_LIBS})
    endif()
endif()

# ── CUDA ─────────────────────────────────────────────────────
if(ENGINE_WRAPPER_CUDA_CC)
    add_library(EngineBridgeCuda STATIC ${ENGINE_WRAPPER_CUDA_CC})
    target_sources(EngineBridgeCuda PRIVATE ${ENGINE_WRAPPER_CUDA_H})
    target_include_directories(EngineBridgeCuda PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(EngineBridgeCuda PRIVATE
        ENGINE_HAS_CUDA_BRIDGE=${ENGINE_HAS_CUDA_BRIDGE})
    if(CUDAToolkit_FOUND)
        target_include_directories(EngineBridgeCuda PRIVATE
            ${CUDAToolkit_INCLUDE_DIRS})
        if(TARGET CUDA::cudart)
            target_link_libraries(EngineBridgeCuda PRIVATE CUDA::cudart)
        endif()
    endif()
endif()

# ── cuDNN ────────────────────────────────────────────────────
if(ENGINE_WRAPPER_CUDNN_CC)
    add_library(EngineBridgeCudnn STATIC ${ENGINE_WRAPPER_CUDNN_CC})
    target_sources(EngineBridgeCudnn PRIVATE ${ENGINE_WRAPPER_CUDNN_H})
    target_include_directories(EngineBridgeCudnn PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(EngineBridgeCudnn PRIVATE
        ENGINE_HAS_CUDNN_BRIDGE=${ENGINE_HAS_CUDNN_BRIDGE})
    if(CUDNN_INCLUDE_DIR)
        target_include_directories(EngineBridgeCudnn PRIVATE ${CUDNN_INCLUDE_DIR})
    endif()
    if(CUDNN_LIBRARY)
        target_link_libraries(EngineBridgeCudnn PRIVATE ${CUDNN_LIBRARY})
    endif()
endif()

# ── Skia ─────────────────────────────────────────────────────
if(ENGINE_WRAPPER_SKIA_CC)
    add_library(EngineBridgeSkia STATIC ${ENGINE_WRAPPER_SKIA_CC})
    target_sources(EngineBridgeSkia PRIVATE ${ENGINE_WRAPPER_SKIA_H})
    target_include_directories(EngineBridgeSkia PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(EngineBridgeSkia PRIVATE
        ENGINE_HAS_SKIA_BRIDGE=${ENGINE_HAS_SKIA_BRIDGE})
    if(ENGINE_SKIA_INCLUDE_DIRS)
        target_include_directories(EngineBridgeSkia PRIVATE
            ${ENGINE_SKIA_INCLUDE_DIRS})
    endif()
    if(ENGINE_SKIA_LIBRARIES)
        target_link_libraries(EngineBridgeSkia PRIVATE ${ENGINE_SKIA_LIBRARIES})
    endif()
endif()

# ── FFmpeg ───────────────────────────────────────────────────
if(ENGINE_WRAPPER_FFMPEG_CC)
    add_library(EngineBridgeFfmpeg STATIC ${ENGINE_WRAPPER_FFMPEG_CC})
    target_sources(EngineBridgeFfmpeg PRIVATE ${ENGINE_WRAPPER_FFMPEG_H})
    target_include_directories(EngineBridgeFfmpeg PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(EngineBridgeFfmpeg PRIVATE
        ENGINE_HAS_FFMPEG_BRIDGE=${ENGINE_HAS_FFMPEG_BRIDGE}
        ENGINE_HAS_FFMPEG_AVFILTER=${ENGINE_HAS_FFMPEG_AVFILTER}
        ENGINE_HAS_FFMPEG_AVDEVICE=${ENGINE_HAS_FFMPEG_AVDEVICE})
    if(ENGINE_FFMPEG_INCLUDE_DIRS)
        target_include_directories(EngineBridgeFfmpeg PRIVATE
            ${ENGINE_FFMPEG_INCLUDE_DIRS})
    endif()
    if(ENGINE_FFMPEG_LIBRARIES)
        target_link_libraries(EngineBridgeFfmpeg PRIVATE ${ENGINE_FFMPEG_LIBRARIES})
    endif()
endif()

# ── ANGLE ────────────────────────────────────────────────────
if(ENGINE_WRAPPER_ANGLE_CC)
    add_library(EngineBridgeAngle STATIC ${ENGINE_WRAPPER_ANGLE_CC})
    target_sources(EngineBridgeAngle PRIVATE ${ENGINE_WRAPPER_ANGLE_H})
    target_include_directories(EngineBridgeAngle PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(EngineBridgeAngle PRIVATE
        ENGINE_HAS_ANGLE_BRIDGE=${ENGINE_HAS_ANGLE_BRIDGE})
    if(ANGLE_INCLUDE_DIR)
        target_include_directories(EngineBridgeAngle PRIVATE ${ANGLE_INCLUDE_DIR})
    endif()
    if(ANGLE_EGL_LIBRARY)
        target_link_libraries(EngineBridgeAngle PRIVATE ${ANGLE_EGL_LIBRARY})
    endif()
    if(ANGLE_GLESV2_LIBRARY)
        target_link_libraries(EngineBridgeAngle PRIVATE ${ANGLE_GLESV2_LIBRARY})
    endif()
endif()
