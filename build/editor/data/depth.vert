#version 430 core
layout (location = 0) in vec3 iPos;
layout (location = 1) in vec2 iTexCoords;

layout (location = 1) uniform mat4 model;
layout (location = 2) uniform mat4 lightSpaceMatrix;

out vec2 TexCoords;

void main() {
    TexCoords = iTexCoords;
    gl_Position = lightSpaceMatrix * model * vec4(iPos, 1.0);
}