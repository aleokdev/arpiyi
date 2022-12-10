cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(arpiyi-codegen src/main.cpp src/parser.cpp)
set_property(TARGET arpiyi-codegen PROPERTY CXX_STANDARD 17)
target_compile_features(arpiyi-codegen PRIVATE cxx_std_17)
target_link_libraries(arpiyi-codegen PRIVATE extlibs)
target_include_directories(arpiyi-codegen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB CODEGEN_DEPENDENCIES ${PROJECT_SOURCE_DIR}/shared/include/assets/*)
set(CODEGEN_GENERATED_FILES ${PROJECT_BINARY_DIR}/shared/include/assets/asset_cg.hpp ${PROJECT_BINARY_DIR}/shared/src/serializer_cg.cpp)
add_custom_command(OUTPUT ${CODEGEN_GENERATED_FILES}
        COMMAND arpiyi-codegen
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS ${CODEGEN_DEPENDENCIES}
        )

add_custom_target(run-arpiyi-codegen
        DEPENDS ${CODEGEN_GENERATED_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Updating codegen...")