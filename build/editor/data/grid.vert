#version 430 core

layout(location = 0) in vec2 iPos;
// layout(location = 1) in vec2 iTexCoords; // Uncomment for generate_wrapping_splitted_quad

layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat4 projection;
layout(location = 5) uniform mat4 view;

out vec2 TexCoords;

void main() {
    TexCoords = iPos; // Change with iTexCoords if using generate_wrapping_splitted_quad

    gl_Position = projection * view * model * vec4(iPos, 0, 1);
}