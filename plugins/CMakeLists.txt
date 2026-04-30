cmake_minimum_required(VERSION 3.10)
project(pxer_example_plugin C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(pxer_example_plugin SHARED pxer_example_plugin.c)

target_include_directories(pxer_example_plugin PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src/native/plugin/pxer
)

target_compile_definitions(pxer_example_plugin PRIVATE PXER_PLUGIN_API_VERSION=1)
