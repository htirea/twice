# Twice
A WIP emulator for the NDS.

## Building
### Overview
The only build configuration I currently regularly test is the example
instructions given in [Linux](#Linux).

#### Dependencies
* A compiler with support for C++20 and C17
* CMake
* SDL2
* Ninja (optional)
* Qt6 (optional)
* libpng (optional)

#### CMake
##### Toolchain files
The following toolchain files can be used to select which compiler to use:
* [`cmake/clang.cmake`](cmake/clang.cmake)
* [`cmake/gcc.cmake`](cmake/gcc.cmake)

##### Options
The options that can be set to configure the build can be found in
[`CMakeLists.txt`](CMakeLists.txt).

### Examples
#### Linux
##### 1. Clone the repository
```bash
git clone --recurse-submodules https://github.com/htirea/twice.git && cd twice
```
##### 2. Generate the build system
```bash
cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=cmake/clang.cmake \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
        -DTWICE_USE_SYSTEM_SDL=1 \
        -DTWICE_ENABLE_SDL_FRONTEND=1 \
        -DENABLE_QT=1 \
        -DENABLE_PNG=1 \
        -DTWICE_USE_SYSTEM_PNG=1 \
        -DTWICE_USE_LTO=1 \
        -DTWICE_INSTALL_DB=1 \
        -GNinja \
        -Bbuild
```

##### 3. Compile
```bash
cmake --build build
```

##### 4. Install
```bash
cmake --install build --prefix ~/opt/twice
```

## Credits
* Martin, for [GBATEK](https://problemkaputt.de/gbatek.htm)
* Arisotura, for various forum posts [1](https://melonds.kuribo64.net/board/thread.php?id=13), [2](https://melonds.kuribo64.net/comments.php?id=85), [3](https://melonds.kuribo64.net/comments.php?id=56)
* StrikerX3, for research on [slope interpolation](https://github.com/StrikerX3/nds-interp)
* Jaklyy, for various 3D related tests / research
* fleroviux, for the horizontal sprite mosaic algorithm

## License
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.

***
Copyright 2023-2024 htirea
