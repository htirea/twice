set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(twice-sdl)

if(TWICE_BUILD_QT)
	add_subdirectory(twice-qt)
endif()
