#version 330 core

uniform sampler2D colorMap;
uniform int maxLod;

in vec2 texCoord;

out vec4 outColor;

void main()
{
    outColor = vec4(0.0);
    for(int lod = 0; lod<maxLod;++lod)
    {
        outColor += textureLod(colorMap, texCoord, float(lod));
    }
        outColor.a = 1.0;


}
