cmake_minimum_required(VERSION 3.22)

# Fixes the 'string REPLACE' error by providing version info first
project(metamodule-rytm-cc VERSION 1.0.0 LANGUAGES C CXX ASM)

# Include the MetaModule SDK
include(metamodule-plugin-sdk/plugin.cmake)

# List all your source files here
add_library(rytm_cc_lib STATIC 
    src/plugin.cpp 
    src/RytmCC.cpp
)

# Finalize the plugin
create_plugin(
    SOURCE_LIB rytm_cc_lib
    PLUGIN_NAME "RytmCC"
)
