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
//out vec3 aNormal;
//out vec3 fragPos;

uniform vec4 clippingPlane;

void main(void)
{
    vec4 positionWorldspace = worldMatrix*vec4(position,1.0);
    gl_ClipDistance[0]=dot(vec4(positionWorldspace.xyz,1.0),clippingPlane);

    //aNormal =(worldMatrix*vec4(normal,1.0)).xyz;
    vTexCoords = texCoords;
    gl_Position = projectionMatrix * worldViewMatrix * vec4(position, 1.0);
}
