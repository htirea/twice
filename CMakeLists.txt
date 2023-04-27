cmake_minimum_required(VERSION 3.26)

project(
	twice
	VERSION 0.1.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

add_subdirectory(src/libtwice)
add_subdirectory(tools)


