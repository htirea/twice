add_executable(twice-sdl
	args.cc
	platform.cc
	main.cc
	screenshot.cc)

target_link_libraries(twice-sdl PRIVATE
	${TWICE_SDL_LIBS}
	twice
)

if(TARGET PNG::PNG)
	target_link_libraries(twice-sdl PRIVATE PNG::PNG)
	target_compile_definitions(twice-sdl PRIVATE TWICE_HAVE_PNG)
endif()

install(TARGETS twice-sdl)
