find_package(SDL2 REQUIRED)

add_executable(twice-sdl
	platform.cc
	main.cc)

target_link_libraries(twice-sdl SDL2::SDL2)
target_link_libraries(twice-sdl twice)
