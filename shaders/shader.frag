#version 450

vec3 lightPosition = vec3(-20, 30, 10);
vec3 lightColor = vec3(1., 1., 1.);
vec3 ambientColor = vec3(0.4, 0.4, 0.4);

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPosition - fragPosition);
    vec3 diffuseColor = lightColor * abs(dot(norm, lightDir));
    outColor = vec4((ambientColor + diffuseColor) * fragColor, 1.0);
}
