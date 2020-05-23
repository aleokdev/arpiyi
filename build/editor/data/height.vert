#version 430 core

in vec3 iPos;
in vec2 iFragCoords;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 projection;
layout(location = 2) uniform mat4 view;

out vec3 vertPos;

void main() {
    vertPos = iPos;
    gl_Position = projection * view * model * vec4(iPos, 1);
}