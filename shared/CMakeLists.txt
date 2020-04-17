cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(arpiyi-shared STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/mesh.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/map.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/tileset.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/texture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/script.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/entity.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/sprite.cpp
        include/global_tile_size.hpp src/global_tile_size.cpp)

add_subdirectory(../lib build/)
target_link_libraries(arpiyi-shared PUBLIC extlibs)
target_include_directories(arpiyi-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)