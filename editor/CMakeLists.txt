cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

add_executable(arpiyi-editor src/main.cpp src/editor/editor_lua_wrapper.cpp src/editor/editor_renderer.cpp src/editor/editor_style.cpp include/plugin_manager.hpp src/plugin_manager.cpp
        include/asset_manager.hpp)
target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(../lib build/)