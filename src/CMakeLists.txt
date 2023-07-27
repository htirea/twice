add_executable(generate-lut
	nds/arm/interpreter/lut_generator.cc)

add_custom_command(
	OUTPUT arm_lut.cc thumb_lut.cc
	COMMAND generate-lut "${CMAKE_CURRENT_BINARY_DIR}/arm_lut.cc" "${CMAKE_CURRENT_BINARY_DIR}/thumb_lut.cc"
	DEPENDS generate-lut)

CHECK_INCLUDE_FILE_CXX(sys/mman.h HAVE_POSIX_MMAP)
if(HAVE_POSIX_MMAP)
	set(FILEMAP_SOURCE libtwice/filemap.cc)
else()
	set(FILEMAP_SOURCE libtwice/filemap_default.cc)
endif()

add_library(twice STATIC
	${FILEMAP_SOURCE}
	libtwice/machine.cc
	nds/arm/arm.cc
	nds/arm/arm7.cc
	nds/arm/arm9.cc
	nds/dma.cc
	nds/arm/interpreter/lut.cc
	nds/gpu/gpu.cc
	nds/gpu/vram.cc
	nds/ipc.cc
	nds/math.cc
	nds/mem/io.cc
	nds/mem/io7.cc
	nds/mem/io9.cc
	nds/nds.cc
	nds/scheduler.cc
	"${CMAKE_CURRENT_BINARY_DIR}/arm_lut.cc"
	"${CMAKE_CURRENT_BINARY_DIR}/thumb_lut.cc"
)

target_include_directories(twice
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_include_directories(twice
	PRIVATE ${PROJECT_SOURCE_DIR}/src)
