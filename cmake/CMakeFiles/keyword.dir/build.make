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
include CMakeFiles/keyword.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/keyword.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/keyword.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/keyword.dir/flags.make

CMakeFiles/keyword.dir/codegen:
.PHONY : CMakeFiles/keyword.dir/codegen

CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o: CMakeFiles/keyword.dir/flags.make
CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o: /home/bea/dpp-bot/src/commands/keyword/keyword.cpp
CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o: CMakeFiles/keyword.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/bea/dpp-bot/cmake/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o -MF CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o.d -o CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o -c /home/bea/dpp-bot/src/commands/keyword/keyword.cpp

CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bea/dpp-bot/src/commands/keyword/keyword.cpp > CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.i

CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bea/dpp-bot/src/commands/keyword/keyword.cpp -o CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.s

# Object files for target keyword
keyword_OBJECTS = \
"CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o"

# External object files for target keyword
keyword_EXTERNAL_OBJECTS =

commands/keyword/libkeyword.so: CMakeFiles/keyword.dir/src/commands/keyword/keyword.cpp.o
commands/keyword/libkeyword.so: CMakeFiles/keyword.dir/build.make
commands/keyword/libkeyword.so: CMakeFiles/keyword.dir/compiler_depend.ts
commands/keyword/libkeyword.so: /usr/lib/libdpp.so
commands/keyword/libkeyword.so: CMakeFiles/keyword.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/bea/dpp-bot/cmake/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library commands/keyword/libkeyword.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/keyword.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/keyword.dir/build: commands/keyword/libkeyword.so
.PHONY : CMakeFiles/keyword.dir/build

CMakeFiles/keyword.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/keyword.dir/cmake_clean.cmake
.PHONY : CMakeFiles/keyword.dir/clean

CMakeFiles/keyword.dir/depend:
	cd /home/bea/dpp-bot/cmake && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bea/dpp-bot /home/bea/dpp-bot /home/bea/dpp-bot/cmake /home/bea/dpp-bot/cmake /home/bea/dpp-bot/cmake/CMakeFiles/keyword.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/keyword.dir/depend

