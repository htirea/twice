add_library(twice STATIC
	machine.cc
)

target_include_directories(twice
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_include_directories(twice
	PRIVATE ${PROJECT_SOURCE_DIR}/src)
