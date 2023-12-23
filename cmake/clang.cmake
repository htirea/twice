if (TOOLCHAIN_FILE_INCLUDED)
	return()
endif()
set(TOOLCHAIN_FILE_INCLUDED true)

set(CMAKE_CXX_FLAGS_INIT " \
-Wall -Wextra -pedantic \
-Wcast-align \
-Wtype-limits \
-Wnon-virtual-dtor \
-Wimplicit-fallthrough")

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
