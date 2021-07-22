cmake_minimum_required(VERSION 3.17)
project(CHIP_8)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 3.3 REQUIRED)

include_directories(Libraries/include)

add_executable(CHIP_8 chip8.cpp main.cpp glad.c)
target_link_libraries(CHIP_8 glfw)