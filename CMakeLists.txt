cmake_minimum_required(VERSION 3.22)

# Ensure the version here matches your Git Tag (v1.0.0)
project(metamodule-rytm-cc VERSION 1.0.0 LANGUAGES C CXX ASM)

# This pulls the build logic from the SDK folder we cloned in the YAML
include(metamodule-plugin-sdk/plugin.cmake)

# List all your source files
add_library(rytm_cc_lib STATIC 
    src/plugin.cpp 
    src/RytmCC.cpp
)

# This tells the compiler to look in the 'src' folder for plugin.hpp
target_include_directories(rytm_cc_lib PRIVATE src)

# This macro from the SDK creates the final .mmplugin file
create_plugin(
    SOURCE_LIB rytm_cc_lib
    PLUGIN_NAME "RytmCC"
)
