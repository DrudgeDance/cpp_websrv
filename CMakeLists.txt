cmake_minimum_required(VERSION 3.15)
project(hot_reload_server)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable hot reloading by building the plugin as a shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add RPATH settings for Linux
if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

find_package(Boost CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Plugin library
add_library(plugin SHARED
    src/plugin.cpp
)
target_include_directories(plugin PUBLIC include)
target_link_libraries(plugin PRIVATE Boost::boost fmt::fmt)

# Main executable
add_executable(server src/main.cpp)
target_include_directories(server PUBLIC include)
target_link_libraries(server PRIVATE Boost::boost fmt::fmt) 