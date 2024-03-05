cmake_minimum_required(VERSION 3.26)

project(
	twice
	VERSION 0.1.0
	LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

include(CheckIncludeFileCXX)
include(GNUInstallDirs)
include(CMakeDependentOption)

set(ENABLE_SDL ON)
cmake_dependent_option(TWICE_USE_SYSTEM_SDL "Use the system SDL" ON "ENABLE_SDL" OFF)
cmake_dependent_option(TWICE_ENABLE_SDL_FRONTEND "Enable SDL frontend" ON "ENABLE_SDL" OFF)
option(ENABLE_QT "Enable Qt frontend" ON)
option(ENABLE_PNG "Enable libpng" ON)
cmake_dependent_option(TWICE_USE_SYSTEM_PNG "Use the system libpng" ON "ENABLE_PNG" OFF)
option(TWICE_USE_LTO "Use link time optimisation" ON)
option(TWICE_INSTALL_DB "Install the database" ON)

if(ENABLE_SDL)
	if(TWICE_USE_SYSTEM_SDL)
		find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
		find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
		SET(TWICE_SDL_LIBS SDL2::SDL2)
	else()
		add_subdirectory(third-party/SDL EXCLUDE_FROM_ALL)
		SET(TWICE_SDL_LIBS SDL2::SDL2-static)
	endif()

	if(TARGET SDL2::SDL2main)
		SET(TWICE_SDL_LIBS SDL2::SDL2main ${TWICE_SDL_LIBS})
	endif()
endif()

if(ENABLE_PNG)
	if(TWICE_USE_SYSTEM_PNG)
		find_package(PNG REQUIRED)
	else()
		add_subdirectory(third-party/zlib EXCLUDE_FROM_ALL)
		add_subdirectory(third-party/libpng EXCLUDE_FROM_ALL)
	endif()
endif()

if(TWICE_USE_LTO)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(TWICE_LINUX TRUE)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(TWICE_WINDOWS TRUE)
endif()

add_subdirectory(src)
add_subdirectory(tools)

if(TWICE_INSTALL_DB)
	install(FILES third-party/nds-db/game-db.json
	DESTINATION ${CMAKE_INSTALL_DATADIR}/twice)
endif()
