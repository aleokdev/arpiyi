#include <api/api.hpp>

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */
#include <iostream>

namespace arpiyi::api {

void map_screen_layer_render_cb() {
    std::cout << "Hello world!" << std::endl;
}

}