#version 430 core

in vec3 iPos;
in vec2 iTexCoords;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 projection;
layout(location = 2) uniform mat4 view;

out vec2 TexCoords;

void main() {
    TexCoords = iTexCoords;

    gl_Position = projection * view * model * vec4(iPos, 1);
}