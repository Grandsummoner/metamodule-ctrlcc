cmake_minimum_required(VERSION 3.22)

project(metamodule-rytm-cc VERSION 1.0.0 LANGUAGES C CXX ASM)

# Must match the 'sdk' folder name in build.yml
include(sdk/plugin.cmake)

add_library(rytm_cc_lib STATIC 
    src/plugin.cpp 
    src/RytmCC.cpp
)

target_include_directories(rytm_cc_lib PRIVATE src)

create_plugin(
    SOURCE_LIB rytm_cc_lib
    PLUGIN_NAME "RytmCC"
)
