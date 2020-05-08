#version 430 core

// Allows mapping to certain UVs. For usage with basic 0-1 UV ranged quads and auto-tileset calculators.

in vec2 TexCoords;

uniform sampler2D tile;// Name hardcoded in renderer_impl_x.cpp. TODO: Add constexpr variable in separate file
uniform sampler2D shadow;// Name hardcoded in renderer_impl_x.cpp. TODO: Add constexpr variable in separate file
layout(location = 6) uniform vec2 uv_start;
layout(location = 7) uniform vec2 uv_end;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

out vec4 FragColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide (not really neccesary for ortho projection, but whatever)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // projCoords = vec3(projCoords.x, 1.0 - projCoords.y, projCoords.z);
    float closestDepth = texture(shadow, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.005;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}

void main() {
    vec2 actual_texcoords = uv_start + (uv_end - uv_start) * fs_in.TexCoords;
    float f_shadow = ShadowCalculation(fs_in.FragPosLightSpace);
    float shadow_force = 0.2;
    FragColor = texture(tile, actual_texcoords).rgba - vec4(vec3(f_shadow), 0) * shadow_force;

    //vec3 projCoords = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
    //projCoords = projCoords * 0.5 + 0.5;
    //FragColor = vec4(vec3(texture(shadow, projCoords.xy).r), 1);
}