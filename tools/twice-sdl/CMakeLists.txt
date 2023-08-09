find_package(SDL2 REQUIRED)
find_package(PNG REQUIRED)

add_executable(twice-sdl
	args.cc
	platform.cc
	main.cc
	screenshot.cc)

target_include_directories(twice-sdl PRIVATE ${SDL_INCLUDE_DIR} ${PNG_INCLUDE_DIR})

target_link_libraries(twice-sdl PRIVATE SDL2::SDL2main SDL2::SDL2 PNG::PNG twice)

install(TARGETS twice-sdl)
