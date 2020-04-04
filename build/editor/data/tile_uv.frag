#version 430 core

// Allows mapping to certain UVs. For usage with basic 0-1 UV ranged quads and auto-tileset calculators.

in vec2 TexCoords;

layout(location = 0) uniform sampler2D tile;
layout(location = 3) uniform vec2 uv_start;
layout(location = 4) uniform vec2 uv_end;

out vec4 FragColor;

void main() {
    vec2 actual_texcoords = uv_start + (uv_end - uv_start) * TexCoords;
    FragColor = texture(tile, actual_texcoords).rgba;
}