add_executable(twice-sdl
	args.cc
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
	ZLIB::ZLIB)

install(TARGETS twice-sdl)
