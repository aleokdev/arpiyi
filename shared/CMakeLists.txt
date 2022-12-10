cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)
file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/src/serializer_cg.cpp)
add_library(arpiyi-shared STATIC
        ${CMAKE_CURRENT_BINARY_DIR}/src/serializer_cg.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/mesh.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/map.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/tileset.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/script.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/entity.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/sprite.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/global_tile_size.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/api/api.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/opengl/renderer_impl_opengl.cpp)

target_link_libraries(arpiyi-shared PUBLIC extlibs)
target_include_directories(arpiyi-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(arpiyi-shared PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if (NOT MSVC) # clang
        target_compile_options(arpiyi-shared PUBLIC -Wno-attributes)
    else () # clang-cl
        target_compile_options(arpiyi-shared PUBLIC -Wno-unknown-attributes)
    endif()
elseif(NOT MSVC)
    # any sane compiler
    target_compile_options(arpiyi-shared PUBLIC -Wno-attributes -Wall)
endif()
target_compile_definitions(arpiyi-shared PUBLIC _USE_MATH_DEFINES) # For M_PI on some platforms such as MSVC

add_dependencies(arpiyi-shared run-arpiyi-codegen)