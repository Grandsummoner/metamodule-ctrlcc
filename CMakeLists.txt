cmake_minimum_required(VERSION 3.22)

if(NOT "${METAMODULE_SDK_DIR}" STREQUAL "")
    message("METAMODULE_SDK_DIR: ${METAMODULE_SDK_DIR}")
else()
    set(METAMODULE_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/metamodule-plugin-sdk")
    message("METAMODULE_SDK_DIR defaulting to: ${METAMODULE_SDK_DIR}")
endif()

include(${METAMODULE_SDK_DIR}/plugin.cmake)

project(RytmCC VERSION 1.0.0 DESCRIPTION "Rytm CC Controller" LANGUAGES C CXX ASM)

add_library(RytmCC STATIC src/RytmCC.cpp)

target_include_directories(RytmCC PRIVATE src)

create_plugin(
    SOURCE_LIB      RytmCC
    PLUGIN_NAME     RytmCC
    PLUGIN_JSON     ${CMAKE_CURRENT_LIST_DIR}/plugin-mm.json
    SOURCE_ASSETS   ${CMAKE_CURRENT_LIST_DIR}/assets
    DESTINATION     ${CMAKE_CURRENT_LIST_DIR}/metamodule-plugins/RytmCC
)
