cmake_minimum_required(VERSION 3.22)

set(METAMODULE_PLUGIN_SDK_DIR "" CACHE PATH "Path to MetaModule Plugin SDK")

if(NOT EXISTS "${METAMODULE_PLUGIN_SDK_DIR}/plugin.cmake")
    message(FATAL_ERROR "METAMODULE_PLUGIN_SDK_DIR not set or invalid: ${METAMODULE_PLUGIN_SDK_DIR}")
endif()

include(${METAMODULE_PLUGIN_SDK_DIR}/plugin.cmake)

project(RytmCC VERSION 1.0.0 LANGUAGES C CXX ASM)

add_library(RytmCC STATIC src/RytmCC.cpp)

target_include_directories(RytmCC PRIVATE src)

create_plugin(
    SOURCE_LIB    RytmCC
    PLUGIN_NAME   RytmCC
    PLUGIN_JSON   ${CMAKE_CURRENT_LIST_DIR}/plugin.json
    DESTINATION   ${CMAKE_BINARY_DIR}
)
