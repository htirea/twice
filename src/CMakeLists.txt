add_executable(generate-lut
	nds/arm/interpreter/lut_generator.cc)

add_custom_command(
	OUTPUT arm_lut.cc thumb_lut.cc
	COMMAND generate-lut "${CMAKE_CURRENT_BINARY_DIR}/arm_lut.cc" "${CMAKE_CURRENT_BINARY_DIR}/thumb_lut.cc"
	DEPENDS generate-lut)

add_library(twice STATIC
	libtwice/filemap.cc
	libtwice/machine.cc
	nds/arm/arm.cc
	nds/arm/arm7.cc
	nds/arm/arm9.cc
	nds/arm/interpreter/lut.cc
	nds/mem/io.cc
	nds/nds.cc
	"${CMAKE_CURRENT_BINARY_DIR}/arm_lut.cc"
	"${CMAKE_CURRENT_BINARY_DIR}/thumb_lut.cc"
)

target_include_directories(twice
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_include_directories(twice
	PRIVATE ${PROJECT_SOURCE_DIR}/src)
