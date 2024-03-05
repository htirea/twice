cmake_minimum_required(VERSION 3.26)

project(
	libpng
	VERSION 1.6.43
	LANGUAGES C)

add_library(png STATIC "")

target_sources(png
	PRIVATE
	src/png.c
	src/pngerror.c
	src/pngget.c
	src/pngmem.c
	src/pngpread.c
	src/pngread.c
	src/pngrio.c
	src/pngrtran.c
	src/pngrutil.c
	src/pngset.c
	src/pngtrans.c
	src/pngwio.c
	src/pngwrite.c
	src/pngwtran.c
	src/pngwutil.c
)

target_include_directories(png PUBLIC include)
target_include_directories(png PRIVATE src)

target_link_libraries(png PUBLIC ZLIB::ZLIB)

add_library(PNG::PNG ALIAS png)
