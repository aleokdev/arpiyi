cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

set(ARPIYI_EDITOR_VERSION 0.1.0)

add_executable(arpiyi-editor src/main.cpp
        src/editor/editor_lua_wrapper.cpp
        src/editor/editor_renderer.cpp
        src/editor/editor_style.cpp
        src/plugin_manager.cpp
        src/tileset_manager.cpp
        src/stb_image.cpp
        src/stb_image_write.cpp
        src/noc_file_dialog.cpp
        src/window_manager.cpp
        src/assets/mesh.cpp
        src/map_manager.cpp
        src/assets/map.cpp
        src/assets/tileset.cpp
        src/startup_dialog.cpp
        src/sprite_manager.cpp
        src/assets/texture.cpp
        src/script_manager.cpp
        src/serializing_manager.cpp include/window_list_menu.hpp src/window_list_menu.cpp)
target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
configure_file(include/project_info.hpp.in include/project_info.hpp @ONLY)
target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

# Use the package PkgConfig to detect GTK+ headers/library files
if(APPLE)
    message(FATAL_ERROR "The arpiyi editor does not support apple devices.")
elseif(UNIX)
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(arpiyi-editor PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_directories(arpiyi-editor PRIVATE ${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    target_link_libraries(arpiyi-editor PRIVATE ${GTK3_LIBRARIES})
endif()
if(WIN32)
# W3 maps to some ultra-high level warning level in clang-cl (so high it triggers -Wc++98-compat) so we turn it down a little bit
target_compile_options(arpiyi-editor PRIVATE -W2 /diagnostics:caret)
elseif(UNIX)
target_compile_options(arpiyi-editor PRIVATE -Wall)
endif()
add_subdirectory(../lib build/)