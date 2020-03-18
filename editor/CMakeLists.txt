cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

add_executable(arpiyi-editor src/main.cpp)
add_subdirectory(../lib build/)