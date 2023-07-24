find_package(SDL2 REQUIRED)

add_executable(twice-sdl
	platform.cc
	main.cc)

target_include_directories(twice-sdl PRIVATE ${SDL_INCLUDE_DIR})

target_link_libraries(twice-sdl PRIVATE SDL2::SDL2main SDL2::SDL2 twice)

install(TARGETS twice-sdl)
