#version 430 core
in vec3 iPos;
in vec2 iTexCoords;

layout (location = 0) uniform mat4 model;
layout (location = 3) uniform mat4 lightSpaceMatrix;

out vec2 TexCoords;

void main() {
    TexCoords = iTexCoords;
    gl_Position = lightSpaceMatrix * model * vec4(iPos, 1.0);
}