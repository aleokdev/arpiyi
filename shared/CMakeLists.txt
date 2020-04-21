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
        src/global_tile_size.cpp)

target_link_libraries(arpiyi-shared PUBLIC extlibs)
target_include_directories(arpiyi-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(arpiyi-shared PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if (NOT MSVC) # clang
        target_compile_options(arpiyi-shared PUBLIC -Wno-attributes)
    else() # clang-cl
        target_compile_options(arpiyi-shared PUBLIC -Wno-unknown-attributes)
    endif()
endif()

add_dependencies(arpiyi-shared run-arpiyi-codegen)