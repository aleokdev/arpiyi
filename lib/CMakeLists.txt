file(GLOB imgui_source ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)

add_library(extlibs STATIC
        ${imgui_source}
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/imgui_impl_opengl3.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/imgui_impl_glfw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imguitextedit/TextEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/stb_image.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/stb_image_write.cpp
        )

add_subdirectory(glad)
add_subdirectory(glfw)
add_subdirectory(anton_math)

target_include_directories(extlibs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/"
        "${CMAKE_CURRENT_SOURCE_DIR}/sol2/single/include/"
        "${CMAKE_CURRENT_SOURCE_DIR}/stb/"
        "${CMAKE_CURRENT_SOURCE_DIR}/noc/"
        "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/imguitextedit"
        )
set_property(TARGET extlibs PROPERTY CXX_STANDARD 17)
target_compile_features(extlibs PUBLIC cxx_std_17)

if (WIN32)
    target_include_directories(extlibs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lua/include)
    target_link_libraries(extlibs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lua53.lib)
elseif (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LUA REQUIRED lua5.3 IMPORTED_TARGET)
    message(STATUS "Using lua ${LUA_VERSION} (Library: ${LUA_LINK_LIBRARIES})")
    target_link_libraries(extlibs PUBLIC PkgConfig::LUA)
endif ()
target_link_libraries(extlibs PUBLIC glad glfw anton_math)

target_link_libraries(extlibs PUBLIC stdc++fs)

target_compile_definitions(extlibs PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

if (WIN32)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND NOT MSVC)
        target_compile_options(glad PRIVATE -Wno-everything)
        target_compile_options(glfw PRIVATE -Wno-everything)
    else ()
        target_compile_options(glad PRIVATE /W0)
        target_compile_options(glfw PRIVATE /W0)
    endif ()
else ()
    target_compile_options(glad PRIVATE -Wno-everything)
    target_compile_options(glfw PRIVATE -Wno-everything)
endif ()

if (WIN32)
    target_compile_definitions(extlibs PUBLIC
            _CRT_SECURE_NO_WARNINGS
            _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING # caused by sol2/single/include/sol/sol.hpp:22511
            # TODO: Proper handling of UNICODE on windows. NOC file dialog causes compiler errors when compiled with unicode.
            # UNICODE
            # _UNICODE
            )

    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        if (NOT MSVC)
            # clang
            target_compile_options(extlibs PUBLIC
                    PRIVATE
                    -Wall -Wextra -pedantic
                    -Werror=return-type
                    -Wno-language-extension-token
                    )
        else ()
            # clang-cl
            # W3 maps to some ultra-high level warning level in clang-cl (so high it triggers -Wc++98-compat) so we turn it down a little bit
            target_compile_options(extlibs PUBLIC -W2 /diagnostics:caret)
        endif ()
    else ()
        # msvc
        target_compile_options(extlibs PUBLIC -W3 /diagnostics:caret)
    endif ()
elseif (UNIX)
    target_compile_options(extlibs PUBLIC -Wall)
endif ()