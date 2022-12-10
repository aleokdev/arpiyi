#version 430 core

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec2 iTexCoords;

layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat4 projection;

layout(location = 4) uniform mat4 lightSpaceMatrix;
layout(location = 5) uniform mat4 view;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} vs_out;

void main()
{
    vs_out.FragPos = vec3(model * vec4(iPos, 1.0));
    vs_out.TexCoords = iTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}