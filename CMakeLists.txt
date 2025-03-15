# Minimum CMake version required
cmake_minimum_required(VERSION 3.22)

# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Set debug and optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wpedantic -Wno-deprecated-declarations")

# Create an executable
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

find_package(DPP REQUIRED)
find_package(Boost REQUIRED COMPONENTS system CONFIG)

# Link the pre-installed DPP package to the main executable
target_link_libraries(${PROJECT_NAME} ${DPP_LIBRARIES} Boost::system)

# Include the DPP directories for the main executable
target_include_directories(${PROJECT_NAME} PRIVATE ${DPP_INCLUDE_DIR})

# Set C++ version for the main executable
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)
