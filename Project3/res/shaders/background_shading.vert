#version 330 core

layout (location=0) in vec3 position;

out vec2 texCoord;

void main()
{
    //From [-1,1] to [0,1]
    texCoord = position.xy*0.5+vec2(0.5);

    gl_Position = vec4(position, 1.0);
    gl_Position.z = 1.0;
}
