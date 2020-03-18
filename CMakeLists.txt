cmake_minimum_required(VERSION 3.15)
project(arpiyi)

set(CMAKE_CXX_STANDARD 17)

add_executable(arpiyi src/main.cpp)
add_subdirectory(lib)