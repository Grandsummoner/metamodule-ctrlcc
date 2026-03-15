cmake_minimum_required(VERSION 3.22)

include(${CMAKE_CURRENT_LIST_DIR}/../metamodule-plugin-sdk/plugin.cmake)

project(RytmCC VERSION 1.0.0 DESCRIPTION "Rytm CC Controller" LANGUAGES C CXX ASM)

add_library(RytmCC STATIC src/RytmCC.cpp)

target_include_directories(RytmCC PRIVATE src)

create_plugin(
    SOURCE_LIB      RytmCC
    PLUGIN_NAME     RytmCC
    SOURCE_ASSETS   ${CMAKE_CURRENT_LIST_DIR}/assets
    DESTINATION     ${CMAKE_CURRENT_LIST_DIR}/metamodule-plugins
)
