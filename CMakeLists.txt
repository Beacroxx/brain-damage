# Minimum CMake version required
cmake_minimum_required(VERSION 3.22)

# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Set debug and optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wpedantic")

# Create an executable
add_executable(${PROJECT_NAME} "src/main.cpp")

find_package(DPP REQUIRED)

# Link the pre-installed DPP package to the main executable
target_link_libraries(${PROJECT_NAME} ${DPP_LIBRARIES})

# Include the DPP directories for the main executable
target_include_directories(${PROJECT_NAME} PRIVATE ${DPP_INCLUDE_DIR})

# Set C++ version for the main executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Collect all .cpp files in the src/commands directory
file(GLOB_RECURSE COMMAND_SOURCES "src/commands/*/*.cpp")

# Iterate over each command source file
foreach(COMMAND_SOURCE ${COMMAND_SOURCES})

  # Extract the command name (first wildcard) from the file path
  get_filename_component(COMMAND_NAME ${COMMAND_SOURCE} DIRECTORY)
  get_filename_component(COMMAND_NAME ${COMMAND_NAME} NAME)

  # Create a shared library for the command
  add_library(${COMMAND_NAME} SHARED ${COMMAND_SOURCE})

  # Link the DPP libraries to the command library
  target_link_libraries(${COMMAND_NAME} ${DPP_LIBRARIES})

  # Include the DPP directories for the command library
  target_include_directories(${COMMAND_NAME} PRIVATE ${DPP_INCLUDE_DIR})

  # Set the output directory for the command library to ./build/commands/<command_name>
  set_target_properties(${COMMAND_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/commands/${COMMAND_NAME}"
  )

  # Set C++ version for the command library
  set_target_properties(${COMMAND_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
  )
endforeach()
