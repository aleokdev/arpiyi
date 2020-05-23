#version 430 core

in vec3 vertPos;
out vec4 FragColor;

void main() {
    float height = vertPos.z / 10.0;
    if(height < 0) {
        FragColor = vec4(0.0, 0.0, 0.0, -height);
    } else {
        FragColor = vec4(1.0, 1.0, 1.0, height);
    }
}
