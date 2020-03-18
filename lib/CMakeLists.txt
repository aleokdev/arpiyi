add_subdirectory(glad)
add_subdirectory(glfw)

file(GLOB imgui_source ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)
target_include_directories(arpiyi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui/ ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/)
target_sources(arpiyi PRIVATE ${imgui_source} imgui/examples/imgui_impl_opengl3.cpp imgui/examples/imgui_impl_glfw.cpp)
target_link_libraries(arpiyi PRIVATE glad glfw)
target_compile_definitions(arpiyi PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)