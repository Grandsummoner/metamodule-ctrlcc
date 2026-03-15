cmake_minimum_required(VERSION 3.22)

# 1. Start the project first
project(metamodule-rytm-cc VERSION 1.0.0 LANGUAGES C CXX ASM)

# 2. THEN include the SDK logic
include(metamodule-plugin-sdk/plugin.cmake)

# 3. THEN define your library
add_library(rytm_cc_lib STATIC 
    src/plugin.cpp 
    src/RytmCC.cpp
)

target_include_directories(rytm_cc_lib PRIVATE src)

# 4. FINALLY create the plugin
create_plugin(
    SOURCE_LIB rytm_cc_lib
    PLUGIN_NAME "RytmCC"
)
