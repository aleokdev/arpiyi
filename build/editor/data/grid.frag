#version 430 core

in vec2 TexCoords;

layout(location = 3) uniform vec4 color;
layout(location = 4) uniform uvec2 grid_cells;

out vec4 FragColor;

float line_width = 0.05f;

void main() {
    vec2 cell_size = 1.0/grid_cells;
    vec2 current_cell = fract(TexCoords*grid_cells);
    float strength = 1.0-(step(line_width, current_cell.x) * step(line_width, current_cell.y));
    strength += step(1.0-line_width/grid_cells.x, TexCoords.x) + step(1.0-line_width/grid_cells.y, TexCoords.y); // Outer edges
    FragColor.rgba = color * min(1,strength);
}