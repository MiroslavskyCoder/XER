cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang)
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(EngineBuilder LANGUAGES C CXX)

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
cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(EngineBuilder LANGUAGES C CXX)

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

install(TARGETS EngineBuilder
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})