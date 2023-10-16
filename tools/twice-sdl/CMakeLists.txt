find_package(SDL2 REQUIRED)
find_package(PNG REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(twice-sdl
	args.cc
	database.cc
	platform.cc
	main.cc
	screenshot.cc)

target_include_directories(twice-sdl PRIVATE ${SDL_INCLUDE_DIR} ${PNG_INCLUDE_DIR})

target_link_libraries(twice-sdl PRIVATE SDL2::SDL2main SDL2::SDL2 PNG::PNG nlohmann_json::nlohmann_json twice)

install(TARGETS twice-sdl)
