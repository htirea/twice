add_library(twice STATIC)

target_include_directories(twice
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_include_directories(twice
	PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(libtwice)
add_subdirectory(nds)
