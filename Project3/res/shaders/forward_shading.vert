#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoords;
layout(location=3) in vec3 tangent;
layout(location=4) in vec3 bitangent;

uniform mat4 projectionMatrix;
uniform mat4 worldViewMatrix;
uniform mat4 worldMatrix;

out vec2 vTexCoords;
out vec3 aNormal;
out vec3 fragPos;

void main(void)
{
    fragPos = vec3(worldMatrix*vec4(position,1.0));
    aNormal =vec3(worldMatrix*vec4(normal,0.0));
    aNormal = normalize(aNormal);
    vTexCoords = texCoords;
    gl_Position = projectionMatrix * worldViewMatrix * vec4(position, 1.0);
}
