#version 430 core

in vec2 TexCoords;

uniform sampler2D tile;// Name hardcoded in renderer_impl_x.cpp. TODO: Add constexpr variable in separate file

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} fs_in;

out vec4 FragColor;

void main() {
    FragColor = texture(tile, fs_in.TexCoords).rgba;
    if (texture(tile, fs_in.TexCoords).a == 0) { gl_FragDepth = 99999; return; }

    gl_FragDepth = gl_FragCoord.z;
}