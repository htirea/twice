cmake_minimum_required(VERSION 3.26)

project(
	zlib
	VERSION 1.3.1
	LANGUAGES C)

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)

add_library(z STATIC "")

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(unistd.h Z_HAVE_UNISTD_H)

set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
	target_compile_definitions(z PRIVATE -D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS)

check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
	target_compile_definitions(z PRIVATE -DNO_FSEEKO)
endif()


configure_file(cmake/zconf.h.cmakein
	${CMAKE_CURRENT_SOURCE_DIR}/include/zconf.h
	@ONLY)

target_sources(z
	PRIVATE
	src/adler32.c
	src/compress.c
	src/crc32.c
	src/deflate.c
	src/gzclose.c
	src/gzlib.c
	src/gzread.c
	src/gzwrite.c
	src/inflate.c
	src/infback.c
	src/inftrees.c
	src/inffast.c
	src/trees.c
	src/uncompr.c
	src/zutil.c)

target_include_directories(z PUBLIC include)
target_include_directories(z PRIVATE src)

add_library(ZLIB::ZLIB ALIAS z)
