add_library(twice STATIC
	arm/arm7.cc
	arm/arm9.cc
	filemap.cc
	machine.cc
	mem/io.cc
	nds.cc
)

target_include_directories(twice
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_include_directories(twice
	PRIVATE ${PROJECT_SOURCE_DIR}/src)
