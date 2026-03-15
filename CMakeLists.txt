cmake_minimum_required(VERSION 3.22)

# This must be at the top
project(metamodule-rytm-cc VERSION 1.0.0 LANGUAGES C CXX ASM)

include(metamodule-plugin-sdk/plugin.cmake)

# You must list BOTH cpp files here
add_library(rytm_cc_lib STATIC 
    src/plugin.cpp 
    src/RytmCC.cpp
)

create_plugin(
    SOURCE_LIB rytm_cc_lib
    PLUGIN_NAME "RytmCC"
)
