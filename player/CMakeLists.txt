cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(arpiyi-player src/main.cpp src/stb_image.cpp src/default_render_impls.cpp src/game_data_manager.cpp src/window_manager.cpp)
target_include_directories(arpiyi-player PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(TARGET arpiyi-player PROPERTY CXX_STANDARD 17)
target_link_libraries(arpiyi-player PRIVATE arpiyi-shared)