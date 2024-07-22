cmake_minimum_required(VERSION 3.19)

project(
  chip8 VERSION 1.0
  DESCRIPTION "chip8 emulator"
  LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find cairo
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)
link_directories(${CAIRO_LIBRARY_DIRS})

# Find SDL2
find_package(SDL2 REQUIRED)

# Target and compile options
add_executable(chip8 src/main.cpp src/chip8.cpp)

target_compile_features(chip8 PRIVATE cxx_std_20)
target_compile_options(chip8 PRIVATE -Wall -Wextra -Wpedantic)

# Link cairo
include_directories(${CAIRO_INCLUDE_DIRS})
target_link_libraries(chip8 ${CAIRO_LIBRARIES})

# Link SDL2
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(chip8 ${SDL2_LIBRARIES})

