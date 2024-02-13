cmake_minimum_required(VERSION 3.26)

project(
	twice
	VERSION 0.1.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

include(CheckIncludeFileCXX)
include(GNUInstallDirs)

option(TWICE_USE_LTO "Use link time optimisation" ON)
option(TWICE_USE_VENDORED_PNG "Use the vendored libpng" OFF)
option(TWICE_USE_VENDORED_SDL "Use the vendored SDL" OFF)
option(TWICE_USE_STATIC_SDL "Link statically to SDL" OFF)
option(TWICE_USE_VENDORED_JSON "Use the vendored json" ON)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(TWICE_LINUX TRUE)
endif()

if(TWICE_USE_VENDORED_PNG)
	add_subdirectory(third-party/zlib EXCLUDE_FROM_ALL)
	set(PNG_SHARED OFF)
	set(PNG_STATIC ON)
	set(PNG_TOOLS OFF)
	set(PNG_BUILD_ZLIB ON)
	set(SKIP_INSTALL_ALL ON)
	add_subdirectory(third-party/libpng EXCLUDE_FROM_ALL)
	set(TWICE_PNG_LIBRARIES png_static)
	target_include_directories(png_static PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}/third-party/libpng)
else()
	find_package(PNG REQUIRED)
	set(TWICE_PNG_LIBRARIES PNG::PNG)
endif()

if(TWICE_USE_VENDORED_SDL)
	add_subdirectory(third-party/SDL EXCLUDE_FROM_ALL)

	if(TARGET SDL2::SDL2main)
		set(TWICE_SDL_LIBRARIES SDL2::SDL2main)
	endif()

	if(TWICE_USE_STATIC_SDL)
		set(TWICE_SDL_LIBRARIES ${TWICE_SDL_LIBRARIES} SDL2::SDL2-static)
	else()
		set(TWICE_SDL_LIBRARIES ${TWICE_SDL_LIBRARIES} SDL2::SDL2)
	endif()
else()
	find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
	find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

	if(TARGET SDL2::SDL2main)
		set(TWICE_SDL_LIBRARIES SDL2::SDL2main)
	endif()

	set(TWICE_SDL_LIBRARIES ${TWICE_SDL_LIBRARIES} SDL2::SDL2)
endif()

if(TWICE_USE_VENDORED_JSON)
	set(JSON_BuildTests OFF CACHE INTERNAL "")
	add_subdirectory(third-party/json)
else()
	find_package(nlohmann_json REQUIRED)
endif()

if(TWICE_USE_LTO)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_subdirectory(src)
add_subdirectory(tools)

install(FILES third-party/nds-db/game-db.json
	DESTINATION ${CMAKE_INSTALL_DATADIR}/twice)
