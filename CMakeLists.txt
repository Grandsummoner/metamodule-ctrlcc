cmake_minimum_required(VERSION 3.24)
project(RytmCC)

set(METAMODULE_PLUGIN_SDK_DIR "" CACHE PATH "Path to MetaModule Plugin SDK")

if(NOT METAMODULE_PLUGIN_SDK_DIR)
    include(FetchContent)
    FetchContent_Declare(
        metamodule-plugin-sdk
        GIT_REPOSITORY https://github.com/4ms/metamodule-plugin-sdk
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(metamodule-plugin-sdk)
    set(METAMODULE_PLUGIN_SDK_DIR ${metamodule-plugin-sdk_SOURCE_DIR})
endif()

include(${METAMODULE_PLUGIN_SDK_DIR}/cmake/plugin.cmake)

project(RytmCC VERSION 1.0.0)

add_plugin(
    PLUGIN_NAME RytmCC
    SOURCES src/RytmCC.cpp
    DESTINATION ${CMAKE_BINARY_DIR}
)
