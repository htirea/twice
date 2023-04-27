add_library(twice STATIC
	filemap.cc
	machine.cc
	nds.cc
)

target_include_directories(twice
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_include_directories(twice
	PRIVATE ${PROJECT_SOURCE_DIR}/src)
