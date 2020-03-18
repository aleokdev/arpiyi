cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

add_executable(arpiyi-editor src/main.cpp src/editor.cpp src/systems/editor_lua_wrapper.cpp src/systems/editor_renderer.cpp include/systems/editor_style.hpp src/systems/editor_style.cpp)
target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(../lib build/)