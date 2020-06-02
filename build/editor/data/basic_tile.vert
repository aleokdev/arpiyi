#version 430 core

in vec3 iPos;
in vec2 iTexCoords;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 projection;
layout(location = 2) uniform mat4 view;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} vs_out;

void main()
{
    vs_out.FragPos = vec3(model * vec4(iPos, 1.0));
    vs_out.TexCoords = iTexCoords;
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}