set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(TWICE_ENABLE_SDL_FRONTEND)
	add_subdirectory(twice-sdl)
endif()

if(ENABLE_QT)
	add_subdirectory(twice-qt)
endif()
