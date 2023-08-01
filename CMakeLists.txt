cmake_minimum_required(VERSION 3.26)

project(
	twice
	VERSION 0.1.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

include(CheckIncludeFileCXX)

add_subdirectory(src)
add_subdirectory(tools)
