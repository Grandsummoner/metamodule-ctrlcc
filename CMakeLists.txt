cmake_minimum_required(VERSION 3.22)

set(METAMODULE_PLUGIN_SDK_DIR "" CACHE PATH "Path to MetaModule Plugin SDK")

include(${METAMODULE_PLUGIN_SDK_DIR}/plugin.cmake)

project(RytmCC VERSION 1.0.0 LANGUAGES C CXX ASM)

add_library(RytmCC STATIC src/RytmCC.cpp)

target_include_directories(RytmCC PRIVATE src)

create_plugin(
    SOURCE_LIB    RytmCC
    PLUGIN_NAME   RytmCC
    DESTINATION   ${CMAKE_BINARY_DIR}
)
