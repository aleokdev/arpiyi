cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(arpiyi-codegen src/main.cpp)
target_link_libraries(arpiyi-codegen PRIVATE extlibs)
add_custom_target(run-arpiyi-codegen
        COMMAND arpiyi-codegen ${PROJECT_BINARY_DIR}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Starting codegen...")
