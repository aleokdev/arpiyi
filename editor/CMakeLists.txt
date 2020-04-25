cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

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
        src/noc_file_dialog.cpp
        src/window_manager.cpp
        src/map_manager.cpp
        src/startup_dialog.cpp
        src/sprite_manager.cpp
        src/script_manager.cpp
        src/serializing_manager.cpp
        src/window_list_menu.cpp
        src/util/process_exec.cpp
        src/project_manager.cpp)
set_property(TARGET arpiyi-editor PROPERTY CXX_STANDARD 17)
target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(arpiyi-editor PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include")

# We need to set a header with information about the project (project_info.hpp). A simple configure_file won't do since
# we require generator expressions for setting ARPIYI_PLAYER_EXECUTABLE_NAME.
file(GENERATE
        OUTPUT "config_$<CONFIG>.out"
        CONTENT "\
#define ARPIYI_EDITOR_VERSION \"${ARPIYI_EDITOR_VERSION}\"\n\
#define ARPIYI_PLAYER_EXECUTABLE_NAME \"$<TARGET_FILE_NAME:arpiyi-player>\""
        )

set(PROJECT_INFO_HEADER_PATH ${CMAKE_CURRENT_BINARY_DIR}/include/project_info.hpp)
add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "config_$<CONFIG>.out" ${PROJECT_INFO_HEADER_PATH}
        VERBATIM
        PRE_BUILD
        OUTPUT ${PROJECT_INFO_HEADER_PATH}
        COMMENT "Creating config.out file ({event: PRE_BUILD}, {filename: ${PROJECT_INFO_HEADER_PATH}})"
)
add_custom_target(generate_config_out DEPENDS ${PROJECT_INFO_HEADER_PATH})
add_dependencies(arpiyi-editor generate_config_out)

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

# Copy player executable to editor dir
add_custom_command(TARGET arpiyi-editor
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:arpiyi-player> ${CMAKE_CURRENT_BINARY_DIR})

add_dependencies(arpiyi-editor arpiyi-player)