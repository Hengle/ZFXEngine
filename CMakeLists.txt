cmake_minimum_required(VERSION 2.8)

project(ZFX)

# option
option(BUILD_EXAMPLES "build examples" ON)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(CMAKE_BUILD_TYPE Debug)

link_directories(${PROJECT_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
link_directories(${CMAKE_SOURCE_DIR}/deps/platform/lib)

include_directories(${CMAKE_SOURCE_DIR}/ZFXEngine/include)
include_directories(${CMAKE_SOURCE_DIR}/deps/platform/include)

add_subdirectory(ZFXEngine)
add_subdirectory(RenderSystem)
add_subdirectory(Examples)