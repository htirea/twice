add_executable(twice-sdl
	args.cc
	database.cc
	platform.cc
	main.cc
	screenshot.cc)

target_include_directories(twice-sdl PRIVATE
	${SDL_INCLUDE_DIR}
	${PNG_INCLUDE_DIR})

target_link_libraries(twice-sdl PRIVATE
	twice
	${TWICE_SDL_LIBRARIES}
	${TWICE_PNG_LIBRARIES}
	${TWICE_Z_LIBRARIES}
	nlohmann_json::nlohmann_json)

install(TARGETS twice-sdl)
