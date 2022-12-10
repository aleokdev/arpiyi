#version 430 core

in vec2 TexCoords;
uniform sampler2D tex;

void main()
{
    // FIXME: this will cause "floating shadows" if the side walls have a transparent texture
    if (texture(tex, TexCoords).a == 0) { gl_FragDepth = 99999; return; }

    gl_FragDepth = gl_FragCoord.z;
}