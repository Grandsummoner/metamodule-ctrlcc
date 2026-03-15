cmake_minimum_required(VERSION 3.22)

# We must set the SDK directory BEFORE the project() call for some SDK versions
set(METAMODULE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/metamodule-plugin-sdk")

# The project line—ensure LANGUAGES includes ASM as required by 4ms
project(metamodule-rytm-cc VERSION 1.0.0 LANGUAGES C CXX ASM)

# Use the absolute path to include the SDK logic
include("${METAMODULE_SDK_DIR}/plugin.cmake")

add_library(rytm_cc_lib STATIC 
    src/plugin.cpp 
    src/RytmCC.cpp
)

target_include_directories(rytm_cc_lib PRIVATE src)

create_plugin(
    SOURCE_LIB rytm_cc_lib
    PLUGIN_NAME "RytmCC"
)
