cmake_minimum_required(VERSION 3.15)
project(arpiyi)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
add_subdirectory(lib)
add_subdirectory(codegen)
add_subdirectory(shared)
add_subdirectory(editor)
add_subdirectory(player)