cmake_minimum_required(VERSION 3.15)
project(UnleashedLoader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR
		"UnleashedLoader must be built 32-bit (the game is a 32-bit PE binary). "
		"Configure with: cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw32.cmake")
endif()

add_library(d3d9loader SHARED
	src/dllmain.cpp
	src/Log.cpp
	src/IniFile.cpp
	src/FileMap.cpp
	src/FileHook.cpp
	src/ModManager.cpp
	UnleashedLoader.def
)

set_target_properties(d3d9loader PROPERTIES
	OUTPUT_NAME "d3d9"
	PREFIX ""
	SUFFIX ".dll"
)

# Stock Windows has no libstdc++/libgcc runtime DLLs, so statically link
# them in - the published DLL must be a single drop-in file.
target_link_options(d3d9loader PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread)
