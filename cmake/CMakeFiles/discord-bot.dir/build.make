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
include CMakeFiles/discord-bot.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/discord-bot.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/discord-bot.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/discord-bot.dir/flags.make

CMakeFiles/discord-bot.dir/codegen:
.PHONY : CMakeFiles/discord-bot.dir/codegen

CMakeFiles/discord-bot.dir/src/main.cpp.o: CMakeFiles/discord-bot.dir/flags.make
CMakeFiles/discord-bot.dir/src/main.cpp.o: /home/bea/dpp-bot/src/main.cpp
CMakeFiles/discord-bot.dir/src/main.cpp.o: CMakeFiles/discord-bot.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/bea/dpp-bot/cmake/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/discord-bot.dir/src/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/discord-bot.dir/src/main.cpp.o -MF CMakeFiles/discord-bot.dir/src/main.cpp.o.d -o CMakeFiles/discord-bot.dir/src/main.cpp.o -c /home/bea/dpp-bot/src/main.cpp

CMakeFiles/discord-bot.dir/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/discord-bot.dir/src/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bea/dpp-bot/src/main.cpp > CMakeFiles/discord-bot.dir/src/main.cpp.i

CMakeFiles/discord-bot.dir/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/discord-bot.dir/src/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bea/dpp-bot/src/main.cpp -o CMakeFiles/discord-bot.dir/src/main.cpp.s

# Object files for target discord-bot
discord__bot_OBJECTS = \
"CMakeFiles/discord-bot.dir/src/main.cpp.o"

# External object files for target discord-bot
discord__bot_EXTERNAL_OBJECTS =

discord-bot: CMakeFiles/discord-bot.dir/src/main.cpp.o
discord-bot: CMakeFiles/discord-bot.dir/build.make
discord-bot: CMakeFiles/discord-bot.dir/compiler_depend.ts
discord-bot: /usr/lib/libdpp.so
discord-bot: CMakeFiles/discord-bot.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/bea/dpp-bot/cmake/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable discord-bot"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/discord-bot.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/discord-bot.dir/build: discord-bot
.PHONY : CMakeFiles/discord-bot.dir/build

CMakeFiles/discord-bot.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/discord-bot.dir/cmake_clean.cmake
.PHONY : CMakeFiles/discord-bot.dir/clean

CMakeFiles/discord-bot.dir/depend:
	cd /home/bea/dpp-bot/cmake && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bea/dpp-bot /home/bea/dpp-bot /home/bea/dpp-bot/cmake /home/bea/dpp-bot/cmake /home/bea/dpp-bot/cmake/CMakeFiles/discord-bot.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/discord-bot.dir/depend

