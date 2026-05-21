cmake_minimum_required(VERSION 3.16)

set(_XER_VCPKG_ROOT_CANDIDATES
    "$ENV{XER_VCPKG_ROOT}"
    "$ENV{VCPKG_ROOT}"
    "${CMAKE_CURRENT_LIST_DIR}/vcpkg"
    "${CMAKE_CURRENT_LIST_DIR}/../vcpkg"
    "${CMAKE_CURRENT_LIST_DIR}/../../vcpkg"
    "${CMAKE_CURRENT_LIST_DIR}/../../../vcpkg")

foreach(_xer_vcpkg_root IN LISTS _XER_VCPKG_ROOT_CANDIDATES)
    if(_xer_vcpkg_root AND EXISTS "${_xer_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
        if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
            set(CMAKE_TOOLCHAIN_FILE "${_xer_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain file")
        endif()
        if(WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
            set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
        endif()
        foreach(_xer_triplet x64-windows x64-windows-static x64-linux)
            if(EXISTS "${_xer_vcpkg_root}/installed/${_xer_triplet}")
                list(APPEND CMAKE_PREFIX_PATH "${_xer_vcpkg_root}/installed/${_xer_triplet}")
            endif()
        endforeach()
        message(STATUS "Using vcpkg root: ${_xer_vcpkg_root}")
        break()
    endif()
endforeach()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(XER_CLANGXX clang++)
    if(XER_CLANGXX)
        set(CMAKE_CXX_COMPILER "${XER_CLANGXX}" CACHE FILEPATH "C++ compiler for XER" FORCE)
    endif()
endif()

function(xer_option_from_env option_name default_value description)
    set(option_default "${default_value}")
    if(DEFINED ENV{${option_name}})
        string(TOUPPER "$ENV{${option_name}}" option_env_value)
        if(option_env_value MATCHES "^(1|ON|TRUE|YES|Y)$")
            set(option_default ON)
        elseif(option_env_value MATCHES "^(0|OFF|FALSE|NO|N)$")
            set(option_default OFF)
        endif()
    endif()
    option(${option_name} "${description}" ${option_default})
endfunction()

xer_option_from_env(ENABLE_CUDA OFF "Enable CUDA language, CUDA sources, and CUDA bridge targets")
xer_option_from_env(ENABLE_CUDNN OFF "Enable cuDNN sources and bridge targets")
xer_option_from_env(ENABLE_AI OFF "Enable AI and model-builder sources")
xer_option_from_env(ENABLE_V8 OFF "Enable V8/libnode JavaScript runtime and XER executable")

if(ENABLE_CUDNN AND NOT ENABLE_CUDA)
    message(FATAL_ERROR "ENABLE_CUDNN=ON requires ENABLE_CUDA=ON")
endif()

if(ENABLE_CUDA)
    set(CMAKE_CUDA_COMPILER clang++)
    set(CMAKE_CUDA_HOST_COMPILER clang++)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --cuda-path=/usr/local/cuda --offload-arch=sm_86")
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES OR CMAKE_CUDA_ARCHITECTURES STREQUAL "")
        set(CMAKE_CUDA_ARCHITECTURES "75;86;90")
    endif()
    project(XER LANGUAGES C CXX CUDA)
else()
    project(XER LANGUAGES C CXX)
endif()


include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies ─────────────────────────────────────────────
include(cmake/deps/system.cmake)
include(cmake/deps/optional.cmake)

# ── Sources ───────────────────────────────────────────────────
include(cmake/sources.cmake)

# ── Targets ───────────────────────────────────────────────────
include(cmake/targets/async_io.cmake)
include(cmake/targets/bridges.cmake)
include(cmake/targets/javascript.cmake)
include(cmake/targets/qt6_bridge.cmake)
include(cmake/targets/main.cmake)

if(TARGET XER)
    install(TARGETS XER
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()