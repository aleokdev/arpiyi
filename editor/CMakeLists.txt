cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

add_executable(arpiyi-editor src/main.cpp src/editor/editor_lua_wrapper.cpp src/editor/editor_renderer.cpp src/editor/editor_style.cpp include/plugin_manager.hpp src/plugin_manager.cpp
        include/asset_manager.hpp src/tileset_manager.cpp src/stb_image.cpp src/noc_file_dialog.cpp src/window_manager.cpp)
target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Use the package PkgConfig to detect GTK+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)
target_include_directories(arpiyi-editor PRIVATE ${GTK3_INCLUDE_DIRS})
target_link_directories(arpiyi-editor PRIVATE ${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})
target_link_libraries(arpiyi-editor PRIVATE ${GTK3_LIBRARIES})
add_subdirectory(../lib build/)