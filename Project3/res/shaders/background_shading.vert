#version 330 core

layout (location=0) in vec3 position;

void main()
{
    gl_Position = vec4(position, 1.0);
    gl_Position.z = 1.0;
}
