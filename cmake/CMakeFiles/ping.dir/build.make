# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.31

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/bea/dpp-bot

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/bea/dpp-bot/cmake

# Include any dependencies generated for this target.
include CMakeFiles/ping.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/ping.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/ping.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ping.dir/flags.make

CMakeFiles/ping.dir/codegen:
.PHONY : CMakeFiles/ping.dir/codegen

CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o: CMakeFiles/ping.dir/flags.make
CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o: /home/bea/dpp-bot/src/commands/ping/ping.cpp
CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o: CMakeFiles/ping.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/bea/dpp-bot/cmake/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o -MF CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o.d -o CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o -c /home/bea/dpp-bot/src/commands/ping/ping.cpp

CMakeFiles/ping.dir/src/commands/ping/ping.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/ping.dir/src/commands/ping/ping.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bea/dpp-bot/src/commands/ping/ping.cpp > CMakeFiles/ping.dir/src/commands/ping/ping.cpp.i

CMakeFiles/ping.dir/src/commands/ping/ping.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/ping.dir/src/commands/ping/ping.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bea/dpp-bot/src/commands/ping/ping.cpp -o CMakeFiles/ping.dir/src/commands/ping/ping.cpp.s

# Object files for target ping
ping_OBJECTS = \
"CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o"

# External object files for target ping
ping_EXTERNAL_OBJECTS =

commands/ping/libping.so: CMakeFiles/ping.dir/src/commands/ping/ping.cpp.o
commands/ping/libping.so: CMakeFiles/ping.dir/build.make
commands/ping/libping.so: CMakeFiles/ping.dir/compiler_depend.ts
commands/ping/libping.so: /usr/lib/libdpp.so
commands/ping/libping.so: CMakeFiles/ping.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/bea/dpp-bot/cmake/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library commands/ping/libping.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ping.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ping.dir/build: commands/ping/libping.so
.PHONY : CMakeFiles/ping.dir/build

CMakeFiles/ping.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ping.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ping.dir/clean

CMakeFiles/ping.dir/depend:
	cd /home/bea/dpp-bot/cmake && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bea/dpp-bot /home/bea/dpp-bot /home/bea/dpp-bot/cmake /home/bea/dpp-bot/cmake /home/bea/dpp-bot/cmake/CMakeFiles/ping.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/ping.dir/depend

