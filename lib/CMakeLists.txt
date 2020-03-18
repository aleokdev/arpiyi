add_subdirectory(glad)
add_subdirectory(glfw)

file(GLOB imgui_source ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)
target_include_directories(arpiyi-editor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/
        ${CMAKE_CURRENT_SOURCE_DIR}/sol2/single/include
        )
target_sources(arpiyi-editor PRIVATE ${imgui_source} imgui/examples/imgui_impl_opengl3.cpp imgui/examples/imgui_impl_glfw.cpp)
target_link_libraries(arpiyi-editor PRIVATE glad glfw lua)
target_compile_definitions(arpiyi-editor PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)