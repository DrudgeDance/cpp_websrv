cmake_minimum_required(VERSION 3.15)
project(hot_reload_server)

# Set base output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# RPATH settings for Unix systems
if(UNIX)
    if(APPLE)
        set(CMAKE_INSTALL_RPATH "@executable_path")
    else()
        set(CMAKE_INSTALL_RPATH "$ORIGIN")
    endif()
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

find_package(Boost CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Endpoints
file(GLOB ENDPOINT_SOURCES "src/endpoints/*.cpp")
foreach(ENDPOINT_SOURCE ${ENDPOINT_SOURCES})
    get_filename_component(ENDPOINT_NAME ${ENDPOINT_SOURCE} NAME_WE)
    add_library(${ENDPOINT_NAME} SHARED ${ENDPOINT_SOURCE})
    set_target_properties(${ENDPOINT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/endpoints
    )
    target_include_directories(${ENDPOINT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${ENDPOINT_NAME} PRIVATE fmt::fmt)
endforeach()

# Routers
file(GLOB ROUTER_SOURCES "src/routers/*.cpp")
foreach(ROUTER_SOURCE ${ROUTER_SOURCES})
    get_filename_component(ROUTER_NAME ${ROUTER_SOURCE} NAME_WE)
    add_library(${ROUTER_NAME} SHARED ${ROUTER_SOURCE})
    set_target_properties(${ROUTER_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/routers
    )
    target_include_directories(${ROUTER_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${ROUTER_NAME} PRIVATE fmt::fmt)
endforeach()

# Controllers
file(GLOB CONTROLLER_SOURCES "src/controllers/*.cpp")
foreach(CONTROLLER_SOURCE ${CONTROLLER_SOURCES})
    get_filename_component(CONTROLLER_NAME ${CONTROLLER_SOURCE} NAME_WE)
    add_library(${CONTROLLER_NAME} SHARED ${CONTROLLER_SOURCE})
    set_target_properties(${CONTROLLER_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/controllers
    )
    target_include_directories(${CONTROLLER_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${CONTROLLER_NAME} PRIVATE fmt::fmt)
endforeach()

# Main application manager
add_library(manager SHARED src/Manager.cpp)
target_include_directories(manager PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(manager PRIVATE fmt::fmt)

# Main executable
add_executable(server src/main.cpp)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(server PRIVATE Boost::boost fmt::fmt) 