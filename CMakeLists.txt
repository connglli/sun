cmake_minimum_required(VERSION 3.20)
project(Sun VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Export compile commands for clangd/IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Module path for custom Find modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Dependencies
include(FetchContent)

# Pugixml for XML parsing
FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG v1.14
)
FetchContent_MakeAvailable(pugixml)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Subdirectories
add_subdirectory(src)
add_subdirectory(tools)

# Testing
enable_testing()
add_subdirectory(tests)

# Summary
message(STATUS "")
message(STATUS "Sun Translation Validation Toolkit")
message(STATUS "===================================")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "")
