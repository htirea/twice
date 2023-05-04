#!/bin/bash

cmake \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_TOOLCHAIN_FILE=cmake/clang.cmake \
	-Bbuild
