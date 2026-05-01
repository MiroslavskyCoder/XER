cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CUDA_COMPILER clang++) 
set(CMAKE_CUDA_HOST_COMPILER clang++)
 
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --cuda-path=/usr/local/cuda --offload-arch=sm_86")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 17)
 
set(CMAKE_CUDA_ARCHITECTURES "75;86;90")

project(XER LANGUAGES C CXX CUDA)

include(GNUInstallDirs)
  
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(absl REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(ICU COMPONENTS uc i18n QUIET)
find_package(SQLite3 QUIET)
find_package(V8 QUIET)  

project(XER LANGUAGES C CXX)

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

install(TARGETS XER
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})