if (TOOLCHAIN_FILE_INCLUDED)
	return()
endif()
set(TOOLCHAIN_FILE_INCLUDED true)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
-Wall -Wextra -pedantic \
-Wcast-align \
-Wtype-limits \
-Wimplicit-fallthrough")

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
