# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/build"

# Include any dependencies generated for this target.
include CMakeFiles/test.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test.dir/flags.make

CMakeFiles/test.dir/sample1.cpp.o: CMakeFiles/test.dir/flags.make
CMakeFiles/test.dir/sample1.cpp.o: ../sample1.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test.dir/sample1.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test.dir/sample1.cpp.o -c "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/sample1.cpp"

CMakeFiles/test.dir/sample1.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test.dir/sample1.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/sample1.cpp" > CMakeFiles/test.dir/sample1.cpp.i

CMakeFiles/test.dir/sample1.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test.dir/sample1.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/sample1.cpp" -o CMakeFiles/test.dir/sample1.cpp.s

# Object files for target test
test_OBJECTS = \
"CMakeFiles/test.dir/sample1.cpp.o"

# External object files for target test
test_EXTERNAL_OBJECTS =

test: CMakeFiles/test.dir/sample1.cpp.o
test: CMakeFiles/test.dir/build.make
test: CMakeFiles/test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test.dir/build: test

.PHONY : CMakeFiles/test.dir/build

CMakeFiles/test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test.dir/clean

CMakeFiles/test.dir/depend:
	cd "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/build" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test" "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test" "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/build" "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/build" "/home/yjh/Nutstore Files/Github_storage/mac_codec_linux/test/build/CMakeFiles/test.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/test.dir/depend
