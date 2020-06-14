#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;

uniform mat4 projectionMatrix;
uniform mat4 worldViewMatrix;

out Data
{
    vec3 positionViewspace;
    vec3 normalViewspace;
}VSOut;

void main(void)
{
    VSOut.positionViewspace = vec3(worldViewMatrix*vec4(position,1.0));
    VSOut.normalViewspace = vec3(worldViewMatrix * vec4(normal,0.0));

    gl_Position = projectionMatrix * vec4(VSOut.positionViewspace,1.0);
}
