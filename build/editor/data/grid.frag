#version 430 core

in vec2 TexCoords;

out vec4 FragColor;

float line_width = 0.02;

void main() {
    vec2 cell_size = vec2(1.0);
    vec2 current_cell = fract(TexCoords);
    float strength = 1.0-(step(line_width, current_cell.x) * step(line_width, current_cell.y));
    strength += step(1.0-line_width, current_cell.x) + step(1.0-line_width, current_cell.y); // Outer edges
    FragColor.rgba = vec4(1,1,1,0.4) * min(1,strength);
}