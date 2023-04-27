if (TOOLCHAIN_FILE_INCLUDED)
	return()
endif()
set(TOOLCHAIN_FILE_INCLUDED true)

include("${CMAKE_CURRENT_LIST_DIR}/gnu-flags.cmake")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
