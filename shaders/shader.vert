#version 450

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main() {
    gl_Position = camera.proj * camera.view * vec4(position, 1.0);
    fragPosition = position;
    fragColor = color;
    fragNormal = normal;
}

