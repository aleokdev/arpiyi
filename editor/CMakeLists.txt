cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ARPIYI_EDITOR_VERSION 0.1.1)

add_executable(arpiyi-editor
        src/main.cpp
        src/editor/editor_lua_wrapper.cpp
        src/editor/editor_renderer.cpp
        src/editor/editor_style.cpp
        src/widgets/pickers.cpp
        src/widgets/inspector.cpp
        src/util/imgui_addons.cpp
        src/plugin_manager.cpp
        src/tileset_manager.cpp
        src/stb_image.cpp
        src/stb_image_write.cpp
        src/noc_file_dialog.cpp
        src/window_manager.cpp
        src/map_manager.cpp
        src/startup_dialog.cpp
        src/sprite_manager.cpp
        src/script_manager.cpp
        src/serializing_manager.cpp
        src/window_list_menu.cpp)
set_property(TARGET arpiyi-editor PROPERTY CXX_STANDARD 17)
target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
configure_file(include/project_info.hpp.in include/project_info.hpp @ONLY)
target_include_directories(arpiyi-editor PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(arpiyi-editor PRIVATE arpiyi-shared)

# Use the package PkgConfig to detect GTK+ headers/library files
if (APPLE)
    message(FATAL_ERROR "The arpiyi editor does not support apple devices.")
elseif (UNIX)
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(arpiyi-editor PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_directories(arpiyi-editor PRIVATE ${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    target_link_libraries(arpiyi-editor PRIVATE ${GTK3_LIBRARIES})
endif ()
