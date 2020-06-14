#version 330 core

in vec2 vTexCoords;
in vec3 aNormal;

out vec4 finalColor;

uniform sampler2D albedoTexture;

void main(void)
{
    finalColor=vec4(0.0);
    finalColor = texture(albedoTexture,vTexCoords);
}
