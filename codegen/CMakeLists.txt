cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(arpiyi-codegen src/main.cpp src/parser.cpp)
target_include_directories(arpiyi-codegen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/shared/include/assets/asset_cg.hpp
        COMMAND arpiyi-codegen "\"${PROJECT_SOURCE_DIR}/shared/include/assets\""
        DEPENDS ${PROJECT_SOURCE_DIR}/shared/include/assets/*
        )

add_custom_target(run-arpiyi-codegen ALL
        DEPENDS arpiyi-codegen
        COMMENT "Starting codegen...")