add_subdirectory(glad)
add_subdirectory(glfw)
add_subdirectory(glm)

file(GLOB imgui_source ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)
target_include_directories(arpiyi-editor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/"
    "${CMAKE_CURRENT_SOURCE_DIR}/sol2/single/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/stb/"
    "${CMAKE_CURRENT_SOURCE_DIR}/noc/"
    "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/imguitextedit"
)
target_sources(arpiyi-editor PRIVATE ${imgui_source} imgui/examples/imgui_impl_opengl3.cpp imgui/examples/imgui_impl_glfw.cpp ${CMAKE_CURRENT_SOURCE_DIR}/imguitextedit/TextEditor.cpp)

if(WIN32)
    target_include_directories(arpiyi-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lua/include)
    target_link_libraries(arpiyi-editor PRIVATE glad glfw ${CMAKE_CURRENT_SOURCE_DIR}/lua53.lib glm)
else()
    target_link_libraries(arpiyi-editor PRIVATE glad glfw lua glm)
endif()

target_compile_definitions(arpiyi-editor PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)

if(WIN32)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND NOT MSVC)
        target_compile_options(glad PRIVATE -Wno-everything)
        target_compile_options(glfw PRIVATE -Wno-everything)
    else()
        target_compile_options(glad PRIVATE /W0)
        target_compile_options(glfw PRIVATE /W0)
    endif()
else()
    target_compile_options(glad PRIVATE -Wno-everything)
    target_compile_options(glfw PRIVATE -Wno-everything)
endif()
